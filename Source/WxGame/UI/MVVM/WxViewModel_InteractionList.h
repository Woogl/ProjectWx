// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "WxViewModel_InteractionList.generated.h"

class UWxInteractionScannerComponent;
class UWxViewModel_Interaction;

/**
 * 상호작용 목록의 표시 값과 입력 통로. 행과 선택의 주인은 값을 공급하는 쪽이며, 본 VM 은 받은 값을 표시한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InteractionList : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetScanner(UWxInteractionScannerComponent* InScanner);

	/** 선택만 바뀌어도 행 전체를 다시 만든다. */
	void SetRows(const TArray<FText>& Prompts, int32 SelectedIndex);

	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestInteract();

	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestCycle(int32 Delta);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	TArray<TObjectPtr<UWxViewModel_Interaction>> Entries;

private:
	TWeakObjectPtr<UWxInteractionScannerComponent> Scanner;
};
