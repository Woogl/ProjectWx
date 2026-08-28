// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_PlaySound.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_PlaySound::FWxStateTreeTask_PlaySound()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_PlaySound::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry && !Instance.bPlayOnRestore)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, Instance.Sound, Owner->GetActorLocation());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlaySound::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("사운드 재생 ({0})"),
		InstanceData->Sound ? FText::FromString(InstanceData->Sound->GetName()) : INVTEXT("none"));
}
#endif
