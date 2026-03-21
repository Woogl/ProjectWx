// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
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
 * 카메라만 타겟을 추적하며, 캐릭터 회전은 이동 방향을 유지.
 * State.Dead 시 활성화 차단. 다른 어빌리티와 공존 가능.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_LockOn : public UWxAbility
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
	float CameraInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float MaxPitchOffset = 20.f;

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
