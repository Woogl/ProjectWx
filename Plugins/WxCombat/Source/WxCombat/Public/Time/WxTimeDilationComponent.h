// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "WxTimeDilationComponent.generated.h"

/**
 * GameState에 부착되어 Global TimeDilation을 서버 권위로 관리하는 컴포넌트.
 *
 * SetGlobalTimeDilation은 자기 World에만 적용되므로, 멀티플레이에서는
 * 모든 머신이 동일한 값을 가지지 않으면 CharacterMovementComponent의
 * 서버/클라 시뮬레이션이 어긋난다.
 *
 * AWxGameState 생성자에서 CreateDefaultSubobject로 부착된다.
 */
UCLASS()
class WXCOMBAT_API UWxTimeDilationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxTimeDilationComponent();

	/**
	 * WorldContextObject가 속한 World의 GameState에 부착된 컴포넌트를 찾아 위임한다.
	 * 컴포넌트가 없으면 no-op.
	 */
	static void SetGlobalTimeDilationAuthoritative(const UObject* WorldContextObject, float NewDilation);

	/** 서버에서 호출. 비-권위 머신에서 호출되면 무시된다. */
	void SetGlobalTimeDilationAuthoritative(float NewDilation);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_ReplicatedTimeDilation();

	void ApplyTimeDilation(float Dilation);

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTimeDilation, VisibleAnywhere, Category = "Wx|Time")
	float ReplicatedTimeDilation = 1.f;
};
