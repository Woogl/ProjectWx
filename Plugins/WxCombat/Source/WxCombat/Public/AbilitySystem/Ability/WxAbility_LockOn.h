// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_LockOn.generated.h"

class UTargetingPreset;
class UUserWidget;
class UWxAbilityTask_LockOnTarget;

/**
 * 입력으로 켜고 재입력으로 끄며, 대상을 잃으면 재탐색하거나 해제한다.
 *
 * 카메라가 타겟을 추적하고, 락온 태스크가 캐릭터를 타겟 방향으로 부드럽게 회전시킨다(그동안 OrientToMovement는 끈다).
 */
UCLASS()
class WXCOMBAT_API UWxAbility_LockOn : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_LockOn();

	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float CameraInterpSpeed = 5.f;

	/** 캐릭터 몸체가 타겟을 향해 회전하는 보간 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float CharacterInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float CameraPitchOffset = -15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float MaxDistance = 2000.f;

	/** 대상을 잃었을 때(사망·파괴·거리이탈) 가장 가까운 다른 적으로 재탐색한다. 끄면 즉시 해제. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	bool bRetargetOnTargetLost = true;

	/** 락온 중 이 크기 이상으로 시선 입력이 누적되면 그 방향의 적으로 재탐색한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float RetargetLookThreshold = 40.f;

	/** 재탐색 시 시선 방향과 후보 방향의 최소 정렬도(cos). 클수록 시선이 가리키는 쪽에 더 정확히 위치한 적만 선택된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float RetargetMinAlignment = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TSubclassOf<UUserWidget> ReticleWidgetClass;

private:
	UFUNCTION()
	void HandleTargetLost();

	/** 화면상 위치가 시선 방향에 가장 정렬된 지점으로 타겟을 교체한다. */
	UFUNCTION()
	void HandleRetargetRequested(FVector2D ScreenDirection);

	/** 거리순 정렬은 프리셋이 담당한다. */
	void GatherCandidates(TArray<AActor*>& OutCandidates) const;

	UPROPERTY()
	TObjectPtr<UWxAbilityTask_LockOnTarget> LockOnTask;
};
