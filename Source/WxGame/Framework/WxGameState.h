// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

/**
 * 프로젝트 공용 GameState.
 *
 * GlobalTimeDilation을 서버 권위로 관리하고 모든 클라이언트에 동기화한다.
 * SetGlobalTimeDilation은 자기 World에만 적용되므로, 멀티플레이에서는
 * 모든 머신이 동일한 값을 가지지 않으면 CharacterMovementComponent의
 * 서버/클라 시뮬레이션이 어긋난다.
 */
UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AWxGameState();

	/**
	 * 서버에서 호출. ReplicatedTimeDilation을 갱신하고 서버 본인에게도 즉시 적용한다.
	 * 클라이언트는 OnRep_ReplicatedTimeDilation에서 적용된다.
	 */
	void SetGlobalTimeDilationAuthoritative(float NewDilation);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTimeDilation, VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Time")
	float ReplicatedTimeDilation = 1.f;

	UFUNCTION()
	void OnRep_ReplicatedTimeDilation();
};
