// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "WxViewModel_InteractionList.generated.h"

class UWxViewModel_Interaction;

/**
 * 상호작용 목록의 표시 값. 행과 선택의 주인은 값을 공급하는 쪽이며, 본 VM 은 받은 값을 표시한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InteractionList : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetRows(const TArray<FText>& Prompts, int32 SelectedIndex);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	TArray<TObjectPtr<UWxViewModel_Interaction>> Entries;
};
