// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeCondition_MasterAbilityActive.h"

#include "Minion/WxMinionComponent.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

bool FWxStateTreeCondition_MasterAbilityActive::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const UAbilitySystemComponent* MasterASC = Pawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(UWxMinionComponent::GetMaster(*Pawn)) : nullptr;
	if (!MasterASC)
	{
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : MasterASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.IsActive() && Spec.Ability->GetAssetTags().HasAny(Instance.AbilityTags))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
FText FWxStateTreeCondition_MasterAbilityActive::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("주인이 {0} 실행 중"), FText::FromString(InstanceData->AbilityTags.ToStringSimple()));
}
#endif
