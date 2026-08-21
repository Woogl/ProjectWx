// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WxInteractable.h"
#include "WxTriggerDevice.generated.h"

class AWxDevice;
class UStaticMeshComponent;

/**
 * 상호작용하면 연결된 장치들에 Event.Interact 를 보내는 발동 장치 액터(레버·버튼·페달 등).
 * 배선은 발동 장치 → 장치 단방향 저작이다 — 이 액터가 움직일 장치를 직접 지목하므로 하나가 여럿을(1:N), 한 장치가 여러 발동 장치에(N:1) 걸린다.
 * 자체 상태머신도 세이브도 없다 — 상태를 드는 쪽은 대상 장치의 StateTree 다(AWxDevice).
 * 상태별 잠금은 두 갈래다. 지목한 장치의 상태로 갈리면 StateTagRequirements 로 스스로 판정하고, 남이 여닫아야 하면 그 트리의 '상호작용 켜기'(Target 갈래)가 계약으로 토글한다.
 *
 * 생김새와 연출은 이 클래스가 알지 않는다 — 눌리면 전 피어에서 OnTriggered 가 불리고, 파생 BP 가 거기서 손잡이 회전·사운드 같은 것을 재생한다.
 */
UCLASS()
class WXWORLD_API AWxTriggerDevice : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxTriggerDevice();

	//~ Begin IWxInteractable
	virtual bool IsInteractionEnabled() const override;
	virtual void SetInteractionEnabled(bool bEnabled) override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

protected:
	virtual void BeginPlay() override;

	/**
	 * 발동 연출 훅. 전 피어에서 불리며, 파생 BP 가 손잡이 회전(Timeline)·사운드 같은 것을 여기서 재생한다.
	 * 재조작 차단은 이 연출의 길이가 아니라 RetriggerCooldown 이 든다 — 연출을 BP 가 정하는 이상 C++ 이 그 길이를 알 수 없기 때문이다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wx")
	void OnTriggered();

	/** 이 발동 장치가 움직일 장치 액터들(AWxDevice). 눌리면 각 액터에 눌림을 통지한다. 하드 참조라 대상과 함께 로드된다 — 늦은 등록을 따로 다루지 않는 근거다. */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TArray<TObjectPtr<AWxDevice>> LinkedDevices;

	/**
	 * 지목한 장치가 전부 이 요건을 만족하는 상태일 때만 활성이다. 비우면 상태와 무관하게 상시 활성이다.
	 * 층별 엘리베이터 호출 레버가 이것으로 잠긴다 — 1F 레버는 반대 층 태그를 Must Have 로 두어, 엘리베이터가 이미 1F 에 있거나 이동 중이면 후보에서 빠진다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagRequirements StateTagRequirements;

	/** 루트이자 상호작용 영역. 쿼리 콜리전이 곧 활성이고, 꺼도 물리 차단은 유지한다(실체 프롭이라 뚫리면 안 된다). */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** HUD 에 표시할 문구. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FText Prompt;

	/**
	 * 발동 후 이 시간 동안 다시 상호작용할 수 없다. 연출이 겹쳐 재생되거나 대상 장치에 눌림이 연타로 나가는 것을 막는다.
	 * 파생 BP 의 연출 길이와 맞춰 둔다 — 짧으면 연출 도중에 다시 눌리고, 길면 연출이 끝나도 잠깐 먹통이다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0.0"))
	float RetriggerCooldown = 1.f;

private:
	/**
	 * 발동을 전 머신에 알린다(연출 재생 + 쿨다운 시작). 복제 카운터를 쓰지 않는 이유는 재진입 통지와 같다 — 늦게 관련성을 얻은 피어에 초기값이 통째로 「변화」로 보인다.
	 * 유실되면 그 클라의 연출과 로컬 쿨다운만 빠진다 — 실제 재조작 차단은 서버의 활성 검증이 들고 있어 무해하다.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Trigger();

	bool IsOnCooldown() const;

	/** 마지막 발동 시각(로컬 시계). 음수면 발동한 적 없음 — 재조작 게이트는 이 값에서 파생한다. */
	double LastTriggerTime = -1.0;
};
