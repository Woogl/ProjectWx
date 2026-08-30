// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxStateTreeTask_GiveRewards.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxRewardLibrary.h"

FWxStateTreeTask_GiveRewards::FWxStateTreeTask_GiveRewards()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_GiveRewards::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const FTransform OwnerTransform = Owner->GetActorTransform();
	const FTransform SpawnTransform(OwnerTransform.GetRotation(), OwnerTransform.TransformPosition(Instance.SpawnOffset));

	UWxRewardLibrary::GrantReward(Owner, Instance.RewardRow, UGameplayStatics::GetPlayerController(Owner, 0), SpawnTransform, Instance.LaunchVelocity);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_GiveRewards::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FName RewardName = InstanceData->RewardRow.RowName;
	return FText::Format(INVTEXT("보상 지급 ({0})"),
		RewardName.IsNone() ? INVTEXT("none") : FText::FromName(RewardName));
}
#endif
