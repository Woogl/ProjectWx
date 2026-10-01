// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Dialogue.generated.h"

class UWxDialogueSessionComponent;

/** 대화의 표시 값과 진행 입력 통로. 대사 구독은 리졸버가 담당한다. */
UCLASS()
class WXGAME_API UWxViewModel_Dialogue : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetSession(UWxDialogueSessionComponent* InSession);

	UFUNCTION()
	void SetLine(const FText& InSpeaker, const FText& InLine);

	UFUNCTION(BlueprintCallable, Category = "Wx|Dialogue")
	void RequestAdvance();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Wx|Dialogue")
	bool HasSpeaker() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText LineText;

private:
	TWeakObjectPtr<UWxDialogueSessionComponent> Session;
};
