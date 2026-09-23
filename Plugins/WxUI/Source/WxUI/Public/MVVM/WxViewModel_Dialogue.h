// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Dialogue.generated.h"

/** 대화의 표시 값만 보관한다. 세션 연결과 진행 입력은 값을 공급하는 쪽이 담당한다. */
UCLASS()
class WXUI_API UWxViewModel_Dialogue : public UWxViewModel
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetLine(const FText& InSpeaker, const FText& InLine);

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Wx|Dialogue")
	bool HasSpeaker() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText LineText;
};
