// Copyright Woogle. All Rights Reserved.

#include "Quest/WxStateTreeTask_StartNextQuest.h"

#include "GameFramework/Actor.h"
#include "Quest/WxQuestComponent.h"
#include "Quest/WxQuestStateTree.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_StartNextQuest::FWxStateTreeTask_StartNextQuest()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_StartNextQuest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
	if (!QuestComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 빈 지정은 컴포넌트가 무시하므로 체인 종점 처리도 같은 호출로 수렴한다.
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	QuestComponent->RequestActivateQuest(Instance.Quest);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_StartNextQuest::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText NextQuestText = InstanceData->Quest.IsNull() ? INVTEXT("none") : FText::FromString(InstanceData->Quest.GetAssetName());
	return FText::Format(INVTEXT("다음 퀘스트 시작 ({0})"), NextQuestText);
}
#endif
