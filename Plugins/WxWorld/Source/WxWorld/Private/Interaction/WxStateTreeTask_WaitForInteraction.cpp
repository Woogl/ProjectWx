// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_WaitForInteraction.h"

#include "GameFramework/Actor.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxWorldModule.h"

namespace
{
	/** 대기 중인 노드 하나. 완료 통보는 상태가 살아 있는 동안에만 유효한 약한 실행 컨텍스트로 보낸다. */
	struct FWxInteractionWait
	{
		int32 Handle = INDEX_NONE;
		FUniversalObjectLocator Target;
		FStateTreeWeakExecutionContext Context;
	};

	TArray<FWxInteractionWait> InteractionWaits;

	// 재사용하지 않으므로 뒤늦은 해제 요청이 엉뚱한 등록을 걷어가지 않는다.
	int32 NextWaitHandle = 0;
}

FWxStateTreeTask_WaitForInteraction::FWxStateTreeTask_WaitForInteraction()
{
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

	// 오너가 사라진 등록을 이 자리에서 걷어내므로 역순으로 돈다 — 아직 보지 않은 낮은 인덱스는 밀리지 않는다.
	// FinishTask 는 완료 상태만 세우므로 완료가 순회 도중 등록을 걷어가지는 않는다.
	for (int32 Index = InteractionWaits.Num() - 1; Index >= 0; --Index)
	{
		// 해석이나 완료가 등록 배열을 건드려도 이 항목이 매달리지 않도록 복사해 둔다.
		const FWxInteractionWait Wait = InteractionWaits[Index];

		TStrongObjectPtr<UObject> Owner = Wait.Context.GetOwner();
		if (!Owner)
		{
			InteractionWaits.RemoveAt(Index);
			continue;
		}

		// PIE 는 서버·클라 월드가 한 프로세스에 산다. 남의 월드에서 온 통보로 완료되지 않도록 좁힌다.
		if (Owner->GetWorld() == Target->GetWorld() && IsWaitingFor(Wait.Target, Target))
		{
			Wait.Context.FinishTask(EStateTreeFinishTaskType::Succeeded);
		}
	}
}

bool FWxStateTreeTask_WaitForInteraction::IsAwaited(const AActor* Target)
{
	// 조회라 오너가 사라진 등록을 걷어내지는 않는다 — 다음 통보가 치운다.
	for (const FWxInteractionWait& Wait : InteractionWaits)
	{
		TStrongObjectPtr<UObject> Owner = Wait.Context.GetOwner();
		if (Owner && Owner->GetWorld() == Target->GetWorld() && IsWaitingFor(Wait.Target, Target))
		{
			return true;
		}
	}

	return false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitForInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 빈 지정은 어떤 대상과도 맞지 않아 완료될 수 없는 잘못된 조립이다.
	if (Instance.Target.IsEmpty())
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Wait For Interaction: 상호작용을 기다릴 대상이 지정되지 않음."));
	}

	Instance.WaitHandle = NextWaitHandle++;
	InteractionWaits.Add({ Instance.WaitHandle, Instance.Target, Context.MakeWeakExecutionContext() });

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitForInteraction::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	InteractionWaits.RemoveAll([&Instance](const FWxInteractionWait& Wait) { return Wait.Handle == Instance.WaitHandle; });
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
	// 컨텍스트로 대상의 레벨을 주면 엔진의 스트리밍 레벨 역추적 경로에서 바로 풀린다 — 오너인 GameState 를 주면 경로 직접 해석 폴백으로 한 단계 멀어진다.
	return Wanted.SyncFind(Target->GetLevel()) == Target;
}
