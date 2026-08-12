// Copyright Woogle. All Rights Reserved.

#include "Quest/WxStateTreeTask_SetQuestObjective.h"

#include "GameFramework/Actor.h"
#include "Quest/WxQuestComponent.h"
#include "StateTreeExecutionContext.h"
#include "WxQuestModule.h"

FWxStateTreeTask_SetQuestObjective::FWxStateTreeTask_SetQuestObjective()
{
	// 진입·이탈에서만 저널을 건드리므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestObjective::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 컨텍스트 오너(GameState)에서 퀘스트 컴포넌트를 찾는다. 퀘스트 러너 밖 조립이면 null.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
	if (!QuestComponent)
	{
		// 이 태스크는 완료 판정 대상이 아니라 엔진이 반환 상태를 무시한다 — 로그가 유일한 진단 수단이다.
		UE_LOG(LogWxQuest, Warning, TEXT("Set Quest Objective: 오너 %s 에서 퀘스트 컴포넌트를 찾지 못함(퀘스트 러너 밖 조립). 목표가 등록되지 않는다."),
			*GetNameSafe(Context.GetOwner()));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.ObjectiveHandle = QuestComponent->AddObjective(Instance.ObjectiveText);

	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_SetQuestObjective::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr)
	{
		QuestComponent->RemoveObjective(Instance.ObjectiveHandle);
	}
	Instance.ObjectiveHandle = INDEX_NONE;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SetQuestObjective::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText ObjectiveText = InstanceData->ObjectiveText.IsEmpty() ? INVTEXT("none") : InstanceData->ObjectiveText;
	return FText::Format(INVTEXT("Set Quest Objective (\"{0}\")"), ObjectiveText);
}
#endif
