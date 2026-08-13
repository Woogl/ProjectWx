// Copyright Woogle. All Rights Reserved.

#include "Quest/WxStateTreeTask_SetQuestTitle.h"

#include "GameFramework/Actor.h"
#include "Quest/WxQuestComponent.h"
#include "StateTreeExecutionContext.h"
#include "WxQuestModule.h"

FWxStateTreeTask_SetQuestTitle::FWxStateTreeTask_SetQuestTitle()
{
	// 진입 시 1회 등록만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestTitle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 컨텍스트 오너(GameState)에서 퀘스트 컴포넌트를 찾는다. 퀘스트 러너 밖 조립이면 null.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
	if (!QuestComponent)
	{
		UE_LOG(LogWxQuest, Warning, TEXT("Set Quest Title: 오너 %s 에서 퀘스트 컴포넌트를 찾지 못함(퀘스트 러너 밖 조립). 제목이 등록되지 않는다."),
			*GetNameSafe(Context.GetOwner()));
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	QuestComponent->SetQuestTitle(Instance.QuestTitle);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SetQuestTitle::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText QuestTitle = InstanceData->QuestTitle.IsEmpty() ? INVTEXT("none") : InstanceData->QuestTitle;
	return FText::Format(INVTEXT("퀘스트 제목 설정 (\"{0}\")"), QuestTitle);
}
#endif
