// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Interaction.generated.h"

/**
 * 상호작용 하나를 나타내는 뷰모델.
 * UWxViewModel_InteractionList 가 인-레인지 대상 하나당 하나씩 생성/소유한다.
 * ListView 엔트리 위젯이 Prompt(표시 텍스트)와 bSelected(현재 선택 여부)에 바인딩한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Interaction : public UWxViewModel
{
	GENERATED_BODY()

public:
	void SetPrompt(const FText& InPrompt);
	void SetSelected(bool bInSelected);

	/** 표시할 프롬프트 텍스트. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	FText Prompt;

	/** 현재 선택된 항목인지 여부(하이라이트용). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	bool bSelected = false;
};
