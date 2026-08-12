// Copyright Woogle. All Rights Reserved.

#include "Quest/WxStateTreeTask_ActivateNextQuest.h"

#include "GameFramework/Actor.h"
#include "Quest/WxQuestComponent.h"
#include "Quest/WxQuestStateTree.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_ActivateNextQuest::FWxStateTreeTask_ActivateNextQuest()
{
	// 진입 시 1회 예약만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_ActivateNextQuest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 컨텍스트 오너(GameState)에서 퀘스트 컴포넌트를 찾는다. 퀘스트 러너 밖 조립이면 null.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
	if (!QuestComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 빈 지정은 컴포넌트가 무시하므로 체인 종점 처리도 같은 호출로 수렴한다.
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	QuestComponent->RequestActivateQuest(Instance.NextQuest);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ActivateNextQuest::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText NextQuestText = InstanceData->NextQuest.IsNull() ? INVTEXT("none") : FText::FromString(InstanceData->NextQuest.GetAssetName());
	return FText::Format(INVTEXT("Activate Next Quest ({0})"), NextQuestText);
}
#endif
