// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Combo.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

UAnimMontage* UWxAbility_Combo::GetMontage() const
{
	const int32 MontageIndex = ComboIndex == INDEX_NONE ? 0 : ComboIndex;
	return ComboMontages.IsValidIndex(MontageIndex) ? ComboMontages[MontageIndex].Get() : nullptr;
}

float UWxAbility_Combo::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

int32 UWxAbility_Combo::GetNextComboIndex() const
{
	if (!IsActive())
	{
		return 0;
	}

	return ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;
}

void UWxAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TriggerEventData)
	{
		const int32 RequestedIndex = FMath::RoundToInt(TriggerEventData->EventMagnitude);
		ComboIndex = ComboMontages.IsValidIndex(RequestedIndex) ? RequestedIndex : 0;
	}
	else
	{
		// 입력이 아닌 발동(AI·미러링)은 서버 단독이라 자기 단계를 이어 쓴다.
		ComboIndex = ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;
	}

	if (!PlayMontage(GetMontage()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Combo::HandleMontageCompleted()
{
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}
