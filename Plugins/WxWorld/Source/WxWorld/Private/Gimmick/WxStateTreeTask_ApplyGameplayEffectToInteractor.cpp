// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_ApplyGameplayEffectToInteractor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_ApplyGameplayEffectToInteractor::FWxStateTreeTask_ApplyGameplayEffectToInteractor()
{
	// 진입 시 1회 적용하고 끝난다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_ApplyGameplayEffectToInteractor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 적용하지 않는다 — 회복은 발동 순간의 효과라 로드 때 다시 일어나면 안 된다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// GE 적용은 서버 권위 사건이다. 클라는 복제된 어트리뷰트를 추종한다.
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !Gimmick || !Instance.EffectClass)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Gimmick->GetInteractingCharacter());
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
	return FText::Format(INVTEXT("Apply \"{0}\" To Interactor"), EffectText);
}
#endif
