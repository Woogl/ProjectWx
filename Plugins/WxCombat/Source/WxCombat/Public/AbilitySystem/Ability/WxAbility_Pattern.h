// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Pattern.generated.h"

class UAnimMontage;

/**
 * BT/AI가 TryActivateAbility로 직접 발동해 단일 몽타주를 재생한다.
 * 입력·UI 아이콘은 쓰지 않으며, 쿨다운·충전은 WxAbilityBase의 AbilityDataRow로 설정한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Pattern : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Pattern();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	virtual void HandleMontageCompleted() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

private:
	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다. */
	int32 ComboIndex = INDEX_NONE;
};
