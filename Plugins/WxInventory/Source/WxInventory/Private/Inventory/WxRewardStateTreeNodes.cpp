// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxRewardStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxRewardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_GrantReward::FWxStateTreeTask_GrantReward()
{
	// 진입 시 1회 지급만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_GrantReward::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 지급을 재실행하지 않고 곧바로 완료한다(발동 순간에만 지급).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 보상 지급은 서버 권위 사건이라 클라 진입은 노옵(클라는 복제로 픽업/인벤토리를 추종).
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 오너의 보상 컴포넌트를 자동 탐색한다(상자엔 하나뿐). 없으면 잘못 배치된 것이므로 Failed.
	UWxRewardComponent* RewardComponent = Owner->FindComponentByClass<UWxRewardComponent>();
	if (!RewardComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 비-픽업 보상(재화 등)은 로컬 플레이어(0번 컨트롤러)에게 직접 지급한다. 픽업 보상은 대상과 무관하게 월드에 스폰된다.
	RewardComponent->DropRewards(UGameplayStatics::GetPlayerController(Owner, 0));

	// 지급은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_GrantReward::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return INVTEXT("Wx Grant Reward");
}
#endif
