// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Dialogue.generated.h"

/** 대화의 표시 값과 진행 입력 통로. 세션 연결은 값을 공급하는 쪽이 담당한다. */
UCLASS()
class WXUI_API UWxViewModel_Dialogue : public UWxViewModel
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetLine(const FText& InSpeaker, const FText& InLine);

	/** 뷰의 진행 입력을 OnAdvanceRequested 로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Dialogue")
	void RequestAdvance();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Wx|Dialogue")
	bool HasSpeaker() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText LineText;

	FSimpleDelegate OnAdvanceRequested;
};
