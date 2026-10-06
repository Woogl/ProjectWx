// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Quest.h"
#include "UI/MVVM/WxViewModel_QuestObjective.h"

void UWxViewModel_Quest::SetJournal(const FText& InQuestTitle, const TArray<FText>& InObjectiveTexts)
{
	UE_MVVM_SET_PROPERTY_VALUE(QuestTitle, InQuestTitle);

	Objectives.Reset(InObjectiveTexts.Num());
	for (const FText& ObjectiveText : InObjectiveTexts)
	{
		UWxViewModel_QuestObjective* Objective = NewObject<UWxViewModel_QuestObjective>(this);
		Objective->SetObjectiveText(ObjectiveText);
		Objectives.Add(Objective);
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Objectives);
}
