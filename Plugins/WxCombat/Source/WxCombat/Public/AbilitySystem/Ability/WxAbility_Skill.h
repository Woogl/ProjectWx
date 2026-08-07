// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Skill.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 스킬 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력/UI → ActivateAbility → SkillMontages[0] 재생
 *  2. ANS_ComboWindow 구간에 재발동(TryActivateAbility) → 엔진 재발동으로 현재 단계를 끝내고 다음 인덱스 몽타주 재생
 *  3. 터미널 인덱스에서 재발동 → 첫 인덱스로 재시작
 *  4. 콤보 미입력 → 몽타주 완료/중단 시 EndAbility
 *
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)으로 처리한다. 진행 신호는 곧 평범한 TryActivateAbility 재호출이므로
 * 하드웨어 입력과 UI 버튼이 같은 경로를 쓰고, 콤보 윈도우 밖 재발동은 CanActivateAbility가 막는다.
 * 단계마다 재발동되므로 비용/쿨다운(CommitAbility)이 단계마다 새로 적용된다.
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

	/**
	 * 활성 중 재발동(콤보 진행)은 콤보 윈도우 안에서만 허용한다.
	 * 이때 자기 차단(BlockAbilitiesWithTag=Ability.Exclusive)은 무시하되(직후 EndAbility가 해제) 사망/비용/쿨다운은 그대로 판정한다.
	 * 신규 발동은 엔진 순정 경로(Super)를 따른다.
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 순차 재생할 스킬 몽타주 목록. 인덱스 0부터 재생하고, ANS_ComboWindow 구간 재발동 시 다음 인덱스로 전환 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TArray<TObjectPtr<UAnimMontage>> SkillMontages;

private:
	/** 현재 인덱스의 몽타주를 재생한다 */
	void PlayCurrentMontage();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	/**
	 * 현재 재생 중인 SkillMontages 인덱스. INDEX_NONE이면 진행 중인 콤보가 없다(다음 발동은 첫 인덱스부터).
	 * 재발동 사이에는 보존되고, 콤보가 자연 종료되면 몽타주 핸들러가 INDEX_NONE으로 되돌린다.
	 */
	int32 CurrentIndex = INDEX_NONE;
};
