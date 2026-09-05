// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_ApplyGameplayEffectToInteractor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Device/WxDevice.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"
#include "Device/WxDeviceExecutionPolicy.h"

FWxStateTreeTask_ApplyGameplayEffectToInteractor::FWxStateTreeTask_ApplyGameplayEffectToInteractor()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_ApplyGameplayEffectToInteractor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 전이로 들어온 것이 아니면 StateTree 시작·레이트조인이다.
	if (FWxDeviceExecutionPolicy::IsRestoring(Context, Transition))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AWxDevice* Owner = Cast<AWxDevice>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority() || !Instance.EffectClass)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner->GetInteractingCharacter());
	if (!ASC)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(Instance.EffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ApplyGameplayEffectToInteractor::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText EffectText = InstanceData->EffectClass ? FText::FromString(InstanceData->EffectClass->GetName()) : INVTEXT("none");
	return FText::Format(INVTEXT("상호작용자에게 \"{0}\" 적용"), EffectText);
}
#endif
