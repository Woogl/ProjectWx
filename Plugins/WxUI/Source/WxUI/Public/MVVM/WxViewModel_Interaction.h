// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Interaction.generated.h"

/**
 * UWxViewModel_InteractionList 가 인-레인지 대상 하나당 하나씩 생성/소유한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Interaction : public UWxViewModel
{
	GENERATED_BODY()

public:
	void SetPrompt(const FText& InPrompt);
	void SetSelected(bool bInSelected);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	FText Prompt;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	bool bSelected = false;
};
