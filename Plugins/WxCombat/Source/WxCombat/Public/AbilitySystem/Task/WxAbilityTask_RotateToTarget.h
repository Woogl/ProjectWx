// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_RotateToTarget.generated.h"

/**
 * 타겟 액터를 향해 CharacterMovementComponent의 RotationRate로 부드럽게 회전하는 AbilityTask.
 * 회전이 완료되거나 타겟이 유효하지 않으면 자동 종료.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_RotateToTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_RotateToTarget* CreateTask(UGameplayAbility* OwningAbility, AActor* InTargetActor);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	static constexpr float RotationTolerance = 3.f;
};
