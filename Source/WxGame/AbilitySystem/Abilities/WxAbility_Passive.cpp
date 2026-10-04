// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Passive.h"
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"
#include "WxGameplayTags.h"

UWxAbility_Passive::UWxAbility_Passive()
{
	// 항상 서버에서 발행된 GameplayEvent로 트리거되고, 지급 결과는 어트리뷰트 복제로 클라이언트에 닿는다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Passive);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Passive);
}

#if WITH_EDITOR
EDataValidationResult UWxAbility_Passive::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	const uint32 NumErrors = Context.GetNumErrors();

	for (const FAbilityTriggerData& Trigger : AbilityTriggers)
	{
		for (const FAbilityTriggerData& OtherTrigger : AbilityTriggers)
		{
			if (Trigger.TriggerTag != OtherTrigger.TriggerTag && Trigger.TriggerTag.MatchesTag(OtherTrigger.TriggerTag))
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("트리거 %s가 %s의 하위라 한 이벤트에 두 번 발동해 효과가 두 번 걸린다."), *Trigger.TriggerTag.ToString(), *OtherTrigger.TriggerTag.ToString())));
			}
		}
	}

	return CombineDataValidationResults(Result, Context.GetNumErrors() > NumErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif

void UWxAbility_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : TriggeredEffects)
	{
		if (EffectClass)
		{
			ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, EffectClass.GetDefaultObject(), GetAbilityLevel());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
