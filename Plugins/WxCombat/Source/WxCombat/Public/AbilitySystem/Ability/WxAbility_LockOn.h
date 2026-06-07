// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_LockOn.generated.h"

class UTargetingPreset;
class UUserWidget;
class UWxAbilityTask_LockOnTarget;

/**
 * 토글 방식 락온 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → TargetingSubsystem으로 적 탐색 → 락온 태스크 시작
 *  2. 재입력 → InputPressed → EndAbility (토글 해제)
 *  3. 타겟 사망/소멸 → HandleTargetLost → EndAbility (자동 해제)
 *
 * 카메라가 타겟을 추적하고, 락온 중에는 OrientToMovement를 끈 뒤 컨트롤러 yaw를 따르게 하여 캐릭터가 타겟을 바라본다. 해제 시 복구.
 * State.Dead 시 활성화 차단. 다른 어빌리티와 공존 가능.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_LockOn : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_LockOn();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float CameraInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float CameraPitchOffset = -15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float MaxDistance = 2000.f;

	/** 락온 대상에 표시할 조준점 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TSubclassOf<UUserWidget> ReticleWidgetClass;

private:
	UFUNCTION()
	void HandleTargetLost();

	UPROPERTY()
	TObjectPtr<UWxAbilityTask_LockOnTarget> LockOnTask;
};
