// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxRewardStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxGameplayTags.h"
#include "WxRewardLibrary.h"

FWxStateTreeTask_GrantReward::FWxStateTreeTask_GrantReward()
{
	// 진입 시 1회 지급만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_GrantReward::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인) 또는 세이브 복원(호스트가 상태 태그와 함께 보내는 StateTree.Restore 마커)이면 지급을 재실행하지 않고 곧바로 완료한다(발동 순간에만 지급).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid() || Context.HasEventToProcess(WxGameplayTags::StateTree_Restore);
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 보상 지급은 서버 권위 사건이라 클라 진입은 노옵(클라는 복제로 픽업/인벤토리를 추종).
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 픽업 스폰 위치는 오너 회전을 유지하고 위치만 오너 로컬 SpawnOffset 만큼 올린다(드랍이 바닥에 끼지 않게).
	const FTransform OwnerTransform = Owner->GetActorTransform();
	const FTransform SpawnTransform(OwnerTransform.GetRotation(), OwnerTransform.TransformPosition(Instance.SpawnOffset));

	// 비-픽업 보상(재화 등)은 로컬 플레이어(0번 컨트롤러)에게 직접 지급한다.
	// 픽업 보상은 대상과 무관하게 월드에 스폰된다.
	UWxRewardLibrary::GrantReward(Owner, Instance.RewardRow, UGameplayStatics::GetPlayerController(Owner, 0), SpawnTransform, Instance.LaunchVelocity);

	// 지급은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_GrantReward::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 지급할 보상은 RewardRow 의 로우 이름으로 식별한다.
	// 비어 있으면 아무것도 지급하지 않으므로 (none) 으로 표시.
	const FName RewardName = InstanceData->RewardRow.RowName;
	return FText::Format(INVTEXT("Grant Reward ({0})"),
		RewardName.IsNone() ? INVTEXT("(none)") : FText::FromName(RewardName));
}
#endif
