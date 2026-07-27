// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"

#include "WxViewModel_QuestObjective.generated.h"

/**
 * 퀘스트 목표 하나를 나타내는 뷰모델.
 * UWxViewModel_Quest 가 표시 중인 목표 하나당 하나씩 생성/소유한다.
 * ListView 엔트리 위젯이 ObjectiveText 에 바인딩한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_QuestObjective : public UWxViewModel
{
	GENERATED_BODY()

public:
	void SetObjectiveText(const FText& InObjectiveText);

	/** 표시할 목표 문구. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText ObjectiveText;
};
