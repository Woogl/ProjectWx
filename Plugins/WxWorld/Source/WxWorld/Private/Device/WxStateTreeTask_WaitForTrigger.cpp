// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_WaitForTrigger.h"

#include "Device/WxDevice.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

FWxStateTreeTask_WaitForTrigger::FWxStateTreeTask_WaitForTrigger()
{
	bShouldCallTick = false;

	// 대기 중 같은 상태가 재선택되어도 등록을 다시 할 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

void FWxStateTreeTask_WaitForTrigger::GetAcceptedOptions(const AWxDevice& Receiver, const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const
{
	if (const FWxDeviceTriggerRule* AcceptRule = Rule.GetPtr())
	{
		AcceptRule->GetAcceptedOptions(Receiver, Sender, OutOptions);

		return;
	}

	// 「그냥 받는다」는 하나면 충분하다 — 한 버튼이 여러 장치를 밀어도 그 버튼의 목록에 같은 행이 겹치지 않는다.
	if (!OutOptions.ContainsByPredicate([](const FWxInteractionOption& Option) { return Option.Prompt.IsEmpty() && Option.Value == INDEX_NONE; }))
	{
		OutOptions.Add(FWxInteractionOption());
	}
}

EStateTreeRunStatus FWxStateTreeTask_WaitForTrigger::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner());
	if (!Device)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Wait For Trigger: 오너 %s 가 장치가 아니라 작동을 기다릴 수 없음."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

		return EStateTreeRunStatus::Failed;
	}

	Device->BeginWaitForTrigger(*this, Context.MakeWeakExecutionContext());

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitForTrigger::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이는 떠나는 상태의 이탈을 먼저 부르고 새 상태의 진입을 부르므로, 다음 상태가 등록한 것을 걷지 않는다.
	if (AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner()))
	{
		Device->EndWaitForTrigger(*this);
	}
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitForTrigger::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	FText Description = INVTEXT("작동 대기");
	if (bPlayerInteraction)
	{
		Description = bOnlyWhenLinkedAccepts
			? FText::Format(INVTEXT("\"{0}\" 상호작용 대기 (연결 장치가 대기 중일 때만)"), Prompt)
			: FText::Format(INVTEXT("\"{0}\" 상호작용 대기"), Prompt);
	}

	if (const UScriptStruct* RuleStruct = Rule.GetScriptStruct())
	{
		return FText::Format(INVTEXT("{0} — 수락 규칙: {1}"), Description, RuleStruct->GetDisplayNameText());
	}

	return Description;
}
#endif
