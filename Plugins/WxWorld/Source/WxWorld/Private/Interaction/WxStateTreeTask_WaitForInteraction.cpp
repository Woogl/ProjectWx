// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_WaitForInteraction.h"

#include "GameFramework/Actor.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxWorldModule.h"

namespace
{
	/** 상호작용을 기다리는 노드 하나. 완료 통보는 상태가 살아 있는 동안에만 유효한 약한 실행 컨텍스트로 보낸다. */
	struct FWxInteractionWait
	{
		int32 Handle = INDEX_NONE;
		FUniversalObjectLocator Target;
		FStateTreeWeakExecutionContext Context;
	};

	TArray<FWxInteractionWait> InteractionWaits;

	/** 재사용하지 않으므로 뒤늦은 해제 요청이 엉뚱한 등록을 걷어가지 않는다. */
	int32 NextWaitHandle = 0;
}

FWxStateTreeTask_WaitForInteraction::FWxStateTreeTask_WaitForInteraction()
{
	// 완료를 통보로 받으므로 볼 것이 없다.
	bShouldCallTick = false;

	// 대기 중 같은 상태가 재선택되어도 등록을 다시 할 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

void FWxStateTreeTask_WaitForInteraction::NotifyInteracted(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	for (int32 Index = InteractionWaits.Num() - 1; Index >= 0; --Index)
	{
		const FWxInteractionWait& Wait = InteractionWaits[Index];

		// 트리째 사라진(오너 파괴) 등록은 통보할 곳이 없으므로 이 참에 걷어낸다.
		if (!Wait.Context.GetOwner())
		{
			InteractionWaits.RemoveAt(Index);
			continue;
		}

		// 대상 해석을 지금 한다 — 기다리는 동안 스트리밍으로 액터가 새로 만들어졌어도 이 순간의 것과 맞춰 본다.
		if (Wait.Target.SyncFind(Target) == Target)
		{
			// 완료한 노드는 상태를 떠나면서 ExitState 에서 스스로 등록을 걷어간다.
			Wait.Context.FinishTask(EStateTreeFinishTaskType::Succeeded);
		}
	}
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

	FWxInteractionWait& Wait = InteractionWaits.AddDefaulted_GetRef();
	Wait.Handle = Instance.WaitHandle;
	Wait.Target = Instance.Target;
	Wait.Context = Context.MakeWeakExecutionContext();

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitForInteraction::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	for (int32 Index = 0; Index < InteractionWaits.Num(); ++Index)
	{
		if (InteractionWaits[Index].Handle == Instance.WaitHandle)
		{
			InteractionWaits.RemoveAt(Index);
			break;
		}
	}
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitForInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("상호작용 대기 ({0})"), FText::FromString(GetTargetDisplayName(InstanceData->Target)));
}

FString FWxStateTreeTask_WaitForInteraction::GetTargetDisplayName(const FUniversalObjectLocator& Locator) const
{
	if (Locator.IsEmpty())
	{
		return TEXT("unset");
	}

	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
	{
		return Actor->GetActorLabel();
	}

	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
	const FActorLocatorFragment* Payload = nullptr;
	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
	{
		const FString SubPath = Payload->Path.GetSubPathString();
		int32 DotIndex = INDEX_NONE;
		return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
	}

	return TEXT("unresolved");
}
#endif
