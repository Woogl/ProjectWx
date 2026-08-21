// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WxInteractable.h"
#include "WxLeverDevice.generated.h"

class USoundBase;
class UStaticMeshComponent;

/**
 * 상호작용하면 연결된 기믹들에 Event.Interact 를 보내는 레버 장치 액터.
 * 배선은 레버 → 기믹 단방향 저작이다 — 이 액터가 움직일 기믹을 직접 지목하므로 한 레버가 여럿을(1:N), 한 기믹이 여러 레버에(N:1) 걸린다.
 * 상시 활성이며 자체 세이브가 없다 — 상태를 드는 쪽은 기믹의 StateTree 다.
 * 상태별 잠금은 두 갈래다. 지목한 기믹의 상태로 갈리면 이 레버가 GimmickStateRequirements 로 스스로 판정하고, 남이 여닫아야 하면 그 트리의 '상호작용 켜기'(Target 갈래)가 계약으로 토글한다.
 */
UCLASS()
class WXWORLD_API AWxLeverDevice : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxLeverDevice();

	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IWxInteractable
	virtual bool IsInteractionEnabled() const override;
	virtual void SetInteractionEnabled(bool bEnabled) override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

protected:
	virtual void BeginPlay() override;

	/** 이 레버가 움직일 기믹 액터들. 눌리면 각 액터의 기믹 컴포넌트에 눌림을 통지한다. 하드 참조라 기믹과 함께 로드된다 — 늦은 등록을 따로 다루지 않는 근거다. */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TArray<TObjectPtr<AActor>> Gimmicks;

	/**
	 * 지목한 기믹이 전부 이 요건을 만족하는 상태일 때만 활성이다. 비우면 상태와 무관하게 상시 활성이다.
	 * 층별 엘리베이터 호출 레버가 이것으로 잠긴다 — 1F 레버는 반대 층 태그를 Must Have 로 두어, 엘리베이터가 이미 1F 에 있거나 이동 중이면 후보에서 빠진다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagRequirements GimmickStateRequirements;

	/** 루트이자 상호작용 영역. 쿼리 콜리전이 곧 활성이고, 꺼도 물리 차단은 유지한다(실체 프롭이라 뚫리면 안 된다). */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** 당김 연출로 회전하는 손잡이. 콜리전이 없어야 몸체와 나란히 스캔 후보로 잡히지 않는다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	/** HUD 에 표시할 문구. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FText Prompt;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0.1"))
	float PullDuration = 1.f;

	/** 당김 정점에서 손잡이가 휴지 자세에 더할 회전. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FRotator HandlePulledRotation = FRotator(-60.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<USoundBase> PullSound;

private:
	/**
	 * 당김 연출을 전 머신에서 재생한다. 복제 카운터를 쓰지 않는 이유는 재진입 통지와 같다 — 늦게 관련성을 얻은 피어에 초기값이 통째로 「변화」로 보인다.
	 * 유실되면 그 클라의 연출만 빠진다 — 재조작 게이트는 서버 판정이 들고 있어 무해하다.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayPull();

	bool IsPulling() const;

	FRotator HandleRestRotation = FRotator::ZeroRotator;

	/** 당김 연출 시작 시각(로컬 시계). 음수면 당긴 적 없음 — 재조작 게이트는 이 값에서 파생한다. */
	double PullStartTime = -1.0;
};
