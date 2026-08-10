// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxRewardStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxRewardLibrary.h"

FWxStateTreeTask_GiveRewards::FWxStateTreeTask_GiveRewards()
{
	// 진입 시 1회 지급만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
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
	return FText::Format(INVTEXT("Give Rewards ({0})"),
		RewardName.IsNone() ? INVTEXT("none") : FText::FromName(RewardName));
}
#endif
