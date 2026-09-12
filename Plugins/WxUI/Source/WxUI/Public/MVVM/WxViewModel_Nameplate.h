// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "MVVM/WxViewModel.h"
#include "GameplayTagContainer.h"
#include "WxViewModel_Nameplate.generated.h"

class UWidgetComponent;

/** 위젯별 거리·소유자 태그·관찰자 유효 상태를 제공하며 표시 변환은 WBP가 수행한다. */
UCLASS()
class WXUI_API UWxViewModel_Nameplate : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UWidgetComponent* InNameplate);
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Nameplate")
	double Distance = 0.0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Nameplate")
	bool bHasViewer = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Nameplate")
	FGameplayTagContainer OwnedTags;

private:
	bool HandleUpdatePresentation(float DeltaTime);
	TWeakObjectPtr<UWidgetComponent> Nameplate;
	FTSTicker::FDelegateHandle UpdateHandle;
};
