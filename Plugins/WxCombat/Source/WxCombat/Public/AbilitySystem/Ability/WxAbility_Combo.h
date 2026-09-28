// Copyright Woogle. All Rights Reserved.

#pragma once

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Combo.generated.h"

/** 단계는 몽타주 배열로, 각 단계의 방향은 몽타주 섹션으로 구성한다. */
UCLASS(Abstract, HideCategories = ("Wx|Montage"))
class WXCOMBAT_API UWxAbility_Combo : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetMontage() const override;

	/** 콤보 동작만 공격 속도(ASPD)를 탄다. */
	virtual float GetMontagePlayRate() const override;

protected:
	/** 커밋한 뒤 다음 단(마지막 단 뒤에는 첫 단)의 몽타주를 재생한다. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 취소되면 다음 발동은 첫 단부터 시작한다. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 끝까지 재생하면 다음 발동은 첫 단부터 시작한다. */
	virtual void HandleMontageCompleted() override;

	/** 배열 순서가 콤보 순서다. 한 단계도 배열에 하나를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다. */
	int32 ComboIndex = INDEX_NONE;
};
