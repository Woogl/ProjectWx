// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"

#include "WxTimeDilationComponent.generated.h"

/**
 * GameState에 부착되어 Global TimeDilation을 서버 권위로 관리하는 컴포넌트.
 *
 * SetGlobalTimeDilation은 자기 World에만 적용되므로, 멀티플레이에서는
 * 모든 머신이 동일한 값을 가지지 않으면 CharacterMovementComponent의
 * 서버/클라 시뮬레이션이 어긋난다.
 *
 * Lyra 패턴에 맞춰 ModularGameplay의 UGameStateComponent를 상속한다. 부착은 GameMode 가 고른 Experience 에셋의 주입 설정으로 한다.
 */
UCLASS()
class WXCOMBAT_API UWxTimeDilationComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UWxTimeDilationComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Requester를 소유자로 삼아 전역 배율을 설정한다.
	 * Requester가 속한 World의 GameState에 부착된 컴포넌트를 찾아 위임하며, 컴포넌트가 없으면 경고를 남기고 no-op.
	 * 서버에서 호출한다 — 비-권위 머신에서 호출되면 무시되고, 클라이언트에는 복제로 도착한다.
	 *
	 * 소유자는 하나뿐이라 나중 요청이 앞선 요청을 밀어낸다.
	 * Set을 부른 쪽이 Clear도 책임진다 — AbilityTask라면 OnDestroy에 두면 엔진이 종료·취소 양쪽에서 불러 준다.
	 */
	static void SetGlobalTimeDilationAuthoritative(const UObject* Requester, float NewDilation);

	/** Requester가 현재 소유자일 때만 배율을 1로 되돌린다. 남이 소유 중이면 그 연출을 건드리지 않고 무시한다. */
	static void ClearGlobalTimeDilationAuthoritative(const UObject* Requester);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetDilationFrom(const UObject* Requester, float NewDilation);

	void ClearDilationFrom(const UObject* Requester);

	UFUNCTION()
	void OnRep_ReplicatedTimeDilation();

	void ApplyTimeDilation(float Dilation);

	/** WorldContextObject가 속한 World의 GameState에서 컴포넌트를 찾는다. 없으면 경고 후 nullptr. */
	static UWxTimeDilationComponent* FindComponent(const UObject* WorldContextObject);

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTimeDilation, VisibleAnywhere, Category = "Wx|Time")
	float ReplicatedTimeDilation = 1.f;

	/** 현재 배율을 건 요청자. 서버에만 존재하며 복제하지 않는다. */
	TWeakObjectPtr<const UObject> DilationOwner;
};
