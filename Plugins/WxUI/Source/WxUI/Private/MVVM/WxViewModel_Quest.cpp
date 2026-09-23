// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Quest.h"
#include "MVVM/WxViewModel_QuestObjective.h"

void UWxViewModel_Quest::SetJournal(bool bInHasActiveQuest, const FText& InQuestTitle, const TArray<FText>& InObjectiveTexts)
{
	UE_MVVM_SET_PROPERTY_VALUE(bHasActiveQuest, bInHasActiveQuest);
	UE_MVVM_SET_PROPERTY_VALUE(QuestTitle, InQuestTitle);
	RebuildObjectives(InObjectiveTexts);
}

void UWxViewModel_Quest::RebuildObjectives(const TArray<FText>& InObjectiveTexts)
{
	Objectives.Reset(InObjectiveTexts.Num());
	for (const FText& ObjectiveText : InObjectiveTexts)
	{
		UWxViewModel_QuestObjective* Objective = NewObject<UWxViewModel_QuestObjective>(this);
		Objective->SetObjectiveText(ObjectiveText);
		Objectives.Add(Objective);
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Objectives);
}
