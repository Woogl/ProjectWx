// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_SpawnNiagara.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_SpawnNiagara::FWxStateTreeTask_SpawnNiagara()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_SpawnNiagara::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (IsValid(Instance.SpawnedComponent) && !Instance.SpawnedComponent->IsComplete())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.Niagara)
	{
		if (USceneComponent* AttachComponent = Instance.AttachComponent.Resolve(Owner))
		{
			Instance.SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(Instance.Niagara, AttachComponent, Instance.AttachSocketName, Instance.RelativeLocation, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
		}
		else
		{
			Instance.SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Owner, Instance.Niagara, Owner->GetActorTransform().TransformPosition(Instance.RelativeLocation));
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SpawnNiagara::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("나이아가라 스폰 ({0})"),
		InstanceData->Niagara ? FText::FromString(InstanceData->Niagara->GetName()) : INVTEXT("none"));
}
#endif
