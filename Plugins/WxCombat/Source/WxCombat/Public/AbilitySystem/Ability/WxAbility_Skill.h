// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Skill.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
class UAnimMontage;

/**
 * 스킬 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → SkillMontages[0] 재생
 *  2. ANS_ComboWindow 구간 입력 → EndAbility 후 동일 spec 재발동, 다음 인덱스 몽타주 재생
 *  3. 터미널 인덱스(다음 없음)의 ANS_ComboWindow 구간 입력 → 첫 인덱스로 재시작 (EndAbility 후 재발동)
 *  4. 콤보 미입력 → 몽타주 완료/중단 시 EndAbility
 *
 * 콤보 단계마다 재발동되므로 비용/쿨다운(CommitAbility)과 OnActivateEffects가 단계마다 새로 적용된다.
 * 콤보가 끊김 없이 이어지려면 AbilityDataRow에서 단계 사이 간격보다 쿨다운을 짧게 잡거나 최대 충전 수를 단계 수 이상으로 둔다.
 *
 * SkillMontages가 1개뿐이면 진행할 다음 단계가 없어, ComboWindow를 배치하지 않는 한 단일 몽타주만 재생하고 종료한다.
 * 타겟 방향 회전은 ANS_SnapToTarget이 담당.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Skill : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	/** 순차 재생할 스킬 몽타주 목록. 인덱스 0부터 재생하고, ANS_ComboWindow 구간 입력 시 다음 인덱스로 전환 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TArray<TObjectPtr<UAnimMontage>> SkillMontages;

private:
	/** 현재 인덱스의 몽타주를 재생한다 */
	void PlayCurrentMontage();

	/** 콤보 입력 대기 태스크를 시작한다 */
	void WaitForComboInput();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleComboInputPressed(float TimeWaited);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> WaitInputTask;

	/** 현재 재생 중인 SkillMontages 인덱스 */
	int32 CurrentIndex = 0;

	/** 다음 ActivateAbility에서 사용할 콤보 인덱스. INDEX_NONE이면 신규 발동(0부터 시작) */
	int32 NextComboIndex = INDEX_NONE;
};
