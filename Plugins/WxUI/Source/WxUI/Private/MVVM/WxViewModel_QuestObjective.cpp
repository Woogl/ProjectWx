// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_QuestObjective.h"

void UWxViewModel_QuestObjective::SetObjectiveText(const FText& InObjectiveText)
{
	if (ObjectiveText.IdenticalTo(InObjectiveText))
	{
		return;
	}

	ObjectiveText = InObjectiveText;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ObjectiveText);
}
