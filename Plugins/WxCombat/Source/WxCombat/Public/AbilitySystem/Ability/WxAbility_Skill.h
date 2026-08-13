// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Ability/WxComboMontageSelector.h"
#include "WxAbility_Skill.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 아바타 태그로 콤보 세트를 골라 첫 몽타주를 재생하고, State.ComboWindow 구간의 재발동이 같은 세트의 다음 단으로 넘긴다(터미널 단에서는 첫 단으로 되돌아간다).
 *
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)이다.
 * 진행 신호가 평범한 TryActivateAbility 재호출이라 하드웨어 입력과 UI 버튼이 같은 경로를 쓰고, 단계마다 CommitAbility가 새로 걸린다.
 *
 * 몽타주가 1개인 세트는 ComboWindow를 배치하지 않는 한 그 하나만 재생하고 종료한다.
 * 세트 선택과 단계 전진 규칙은 ComboSelector에 있다.
 * 타겟 방향 회전은 ANS_SnapToTarget이 담당.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Skill : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();

	/**
	 * 활성 중 재발동(콤보 진행)은 콤보 윈도우 안에서만 허용하며, 이때 자기 차단은 무시하되 사망·비용·쿨다운은 그대로 판정한다.
	 * 신규 발동은 엔진 순정 경로(Super)를 따른다.
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo", meta = (ShowOnlyInnerProperties))
	FWxComboMontageSelector ComboSelector;

private:
	bool PlayMontage(UAnimMontage* Montage);

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
};
