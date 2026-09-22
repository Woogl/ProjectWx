// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Interaction.generated.h"

/**
 * UWxViewModel_InteractionList 가 스캐너의 행(선택지) 하나당 하나씩 생성/소유한다.
 * 만들어진 뒤 바뀌지 않는다 — 문구나 선택이 바뀌면 목록 VM 이 행을 통째로 다시 만든다.
 */
UCLASS()
class WXUI_API UWxViewModel_Interaction : public UWxViewModel
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	FText Prompt;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	bool bSelected = false;
};
