// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"

#include "WxTimeDilationComponent.generated.h"

/**
 * GameState에 부착돼 Global TimeDilation을 서버 권위로 관리하는 컴포넌트.
 * SetGlobalTimeDilation은 자기 World에만 적용되므로, 모든 머신이 같은 값을 갖지 않으면 CharacterMovement의 서버·클라 시뮬레이션이 어긋난다.
 *
 * 부착은 GameMode가 고른 Experience 에셋의 주입 설정으로 한다.
 */
UCLASS()
class WXCOMBAT_API UWxTimeDilationComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UWxTimeDilationComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * 서버에서 호출한다 — 비권위 머신에서는 무시되고, 클라이언트에는 복제로 도착한다.
	 * 소유자는 Requester 하나뿐이라 나중 요청이 앞선 요청을 밀어내며, Set을 부른 쪽이 Clear도 책임진다.
	 */
	static void SetGlobalTimeDilationAuthoritative(const UObject* Requester, float NewDilation);

	/** Requester가 현재 소유자일 때만 배율을 1로 되돌린다. */
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

	static UWxTimeDilationComponent* FindComponent(const UObject* WorldContextObject);

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTimeDilation, VisibleAnywhere, Category = "Wx|Time")
	float ReplicatedTimeDilation = 1.f;

	/** 서버에만 존재하며 복제하지 않는다. */
	TWeakObjectPtr<const UObject> DilationOwner;
};
