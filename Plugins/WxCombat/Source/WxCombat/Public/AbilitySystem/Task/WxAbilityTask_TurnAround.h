// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_TurnAround.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnTurnAroundCompleted);

/**
 * 액터를 목표 방향으로 부드럽게 회전시키는 AbilityTask.
 * 매 틱 보간하여 회전하며, 목표에 도달하면 OnCompleted를 브로드캐스트.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_TurnAround : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_TurnAround* CreateTask(UGameplayAbility* OwningAbility, FVector InTargetDirection, float InInterpSpeed = 15.f, float InCompletionThreshold = 1.f);

	UPROPERTY()
	FWxOnTurnAroundCompleted OnCompleted;

protected:
	virtual void TickTask(float DeltaTime) override;

private:
	FRotator TargetRotation;
	float InterpSpeed = 15.f;
	float CompletionThresholdSquared = 1.f;
};
