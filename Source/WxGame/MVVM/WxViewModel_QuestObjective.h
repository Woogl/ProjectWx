// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"

#include "WxViewModel_QuestObjective.generated.h"

/**
 * UWxViewModel_Quest 가 표시 중인 목표 하나당 하나씩 생성/소유한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_QuestObjective : public UWxViewModel
{
	GENERATED_BODY()

public:
	void SetObjectiveText(const FText& InObjectiveText);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText ObjectiveText;
};
