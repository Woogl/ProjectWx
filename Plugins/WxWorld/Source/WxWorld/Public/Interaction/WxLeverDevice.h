// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxInteractable.h"
#include "WxLeverDevice.generated.h"

class USoundBase;
class UStaticMeshComponent;
class UWxGimmickStateTreeComponent;

/**
 * 상호작용하면 연결된 기믹에 신호를 보내는 레버 장치 액터.
 * 배선은 기믹 → 레버 단방향 저작이다 — 기믹 컴포넌트가 역할별 링크로 이 액터를 지목하고, 여기는 눌림 통지의 역경로 구독만 런타임에 든다.
 * 활성/잠금은 몸체 메시의 쿼리 콜리전이 전부이며 기본은 꺼짐이다 — 켜는 시점은 구독 기믹의 StateTree 가 정하므로, 세이브 복원도 그 트리의 재적용으로 정합된다(자체 세이브 없음).
 */
UCLASS()
class WXWORLD_API AWxLeverDevice : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxLeverDevice();

	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IWxInteractable
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;
	virtual void SetInteractionEnabled(bool bEnabled) override;
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

	/** 기믹이 역할 상태를 밀어 넣는 진입점. 켤 때 프롬프트를 비워 두면 저작 기본 문구가 쓰인다. */
	void SetDeviceActive(bool bActive, const FText& InPrompt);

	void RegisterSubscriber(UWxGimmickStateTreeComponent* Gimmick);
	void UnregisterSubscriber(UWxGimmickStateTreeComponent* Gimmick);

protected:
	virtual void BeginPlay() override;

	/** 루트이자 상호작용 영역. 쿼리 콜리전이 곧 활성이고, 꺼도 물리 차단은 유지한다(실체 프롭이라 뚫리면 안 된다). */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** 당김 연출로 회전하는 손잡이. 콜리전이 없어야 몸체와 나란히 스캔 후보로 잡히지 않는다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	/** 기믹이 프롬프트를 밀어 주지 않을 때 표시할 기본 문구. */
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

	/** 눌림을 통지받을 기믹 컴포넌트들. 기믹이 BeginPlay 에 걸고 EndPlay 에 푼다 — 어긋난 해제는 약참조가 흡수한다. */
	TArray<TWeakObjectPtr<UWxGimmickStateTreeComponent>> Subscribers;

	/** 기믹이 밀어 넣은 상태별 프롬프트. 각 피어의 ST 가 같은 값으로 수렴시키므로 복제하지 않는다. */
	UPROPERTY(Transient)
	FText PushedPrompt;

	FRotator HandleRestRotation = FRotator::ZeroRotator;

	/** 당김 연출 시작 시각(로컬 시계). 음수면 당긴 적 없음 — 재조작 게이트는 이 값에서 파생한다. */
	double PullStartTime = -1.0;
};
