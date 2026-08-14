// Copyright Woogle. All Rights Reserved.

#include "Quest/WxStateTreeTask_SetQuestObjective.h"

#include "GameFramework/Actor.h"
#include "Quest/WxQuestComponent.h"
#include "StateTreeExecutionContext.h"
#include "WxQuestModule.h"

FWxStateTreeTask_SetQuestObjective::FWxStateTreeTask_SetQuestObjective()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestObjective::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
	if (!QuestComponent)
	{
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
	return FText::Format(INVTEXT("퀘스트 목표 설정 (\"{0}\")"), ObjectiveText);
}
#endif
