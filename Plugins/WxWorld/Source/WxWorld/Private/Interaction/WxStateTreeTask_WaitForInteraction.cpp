// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_WaitForInteraction.h"

#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxWorldModule.h"
#include "StateTreeTask/WxStateTreeWaitRegistry.h"

namespace
{
	TWxStateTreeWaitRegistry<FUniversalObjectLocator> InteractionWaits;
}

FWxStateTreeTask_WaitForInteraction::FWxStateTreeTask_WaitForInteraction()
{
	// 완료를 통보로 받으므로 볼 것이 없다.
	bShouldCallTick = false;

	// 대기 중 같은 상태가 재선택되어도 등록을 다시 할 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

void FWxStateTreeTask_WaitForInteraction::NotifyInteracted(const AActor* Target)
{
	if (!Target)
	{
		return;
	}

	InteractionWaits.FinishMatching(Target->GetWorld(), Target, &IsWaitingFor);
}

bool FWxStateTreeTask_WaitForInteraction::IsAwaited(const AActor* Target)
{
	return InteractionWaits.AnyMatching(Target->GetWorld(), Target, &IsWaitingFor);
}

EStateTreeRunStatus FWxStateTreeTask_WaitForInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 빈 지정은 어떤 대상과도 맞지 않아 완료될 수 없는 잘못된 조립이다.
	if (Instance.Target.IsEmpty())
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Wait For Interaction: 상호작용을 기다릴 대상이 지정되지 않음."));
	}

	Instance.WaitHandle = InteractionWaits.Add(Context, Instance.Target);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitForInteraction::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	InteractionWaits.Remove(Instance.WaitHandle);
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitForInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("상호작용 대기 ({0})"), FWxLocatorUtils::GetDisplayName(InstanceData->Target));
}
#endif

bool FWxStateTreeTask_WaitForInteraction::IsWaitingFor(const FUniversalObjectLocator& Wanted, const AActor* Target)
{
	// 대상 해석을 지금 한다 — 기다리는 동안 스트리밍으로 액터가 새로 만들어졌어도 이 순간의 것과 맞춰 본다.
	// 컨텍스트가 대상의 레벨이면 엔진이 스트리밍 레벨을 역추적하는 1차 경로에서 바로 풀린다.
	// 오너인 GameState 를 주면 그 경로를 못 타고 경로 직접 해석 폴백으로 넘어간다 — 폴백도 WP 런타임 셀 안의 액터를 찾지만 한 단계 멀다.
	return Wanted.SyncFind(Target->GetLevel()) == Target;
}
