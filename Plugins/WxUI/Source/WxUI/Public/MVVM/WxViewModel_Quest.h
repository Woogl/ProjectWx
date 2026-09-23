// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"

#include "WxViewModel_Quest.generated.h"

class UWxViewModel_QuestObjective;

/** 퀘스트 추적 HUD의 표시 데이터. */
UCLASS()
class WXUI_API UWxViewModel_Quest : public UWxViewModel
{
	GENERATED_BODY()

public:
	void SetJournal(bool bInHasActiveQuest, const FText& InQuestTitle, const TArray<FText>& InObjectiveTexts);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	bool bHasActiveQuest = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText QuestTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	TArray<TObjectPtr<UWxViewModel_QuestObjective>> Objectives;

private:
	void RebuildObjectives(const TArray<FText>& InObjectiveTexts);
};
