// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "WxViewModel_Quest.generated.h"

class UWxViewModel_QuestObjective;

/** 퀘스트 추적 HUD의 표시 데이터. */
UCLASS()
class WXGAME_API UWxViewModel_Quest : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetJournal(const FText& InQuestTitle, const TArray<FText>& InObjectiveTexts);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText QuestTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	TArray<TObjectPtr<UWxViewModel_QuestObjective>> Objectives;
};
