// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_QuestObjective.h"

void UWxViewModel_QuestObjective::SetObjectiveText(const FText& InObjectiveText)
{
	UE_MVVM_SET_PROPERTY_VALUE(ObjectiveText, InObjectiveText);
}
