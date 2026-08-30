// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_RotateToTarget.generated.h"

class USceneComponent;
/** 지정된 타겟 방향으로 캐릭터 몸체 Yaw를 보간한다. */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_RotateToTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_RotateToTarget* CreateTask(UGameplayAbility* OwningAbility, USceneComponent* InTarget, float InInterpSpeed = 10.f);

	virtual void TickTask(float DeltaTime) override;
	TWeakObjectPtr<USceneComponent> Target;
	float InterpSpeed = 10.f;
};
