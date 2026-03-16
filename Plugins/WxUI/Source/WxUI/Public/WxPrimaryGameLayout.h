// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "WxPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;

UCLASS(Abstract)
class WXUI_API UWxPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetStack* GetLayerWidgetStack(FGameplayTag LayerTag) const;

	UCommonActivatableWidget* PushWidgetToLayerStack(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UCommonActivatableWidget* PushWidgetInstanceToLayerStack(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Layers", meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Layers", meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuLayer;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Layers", meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Layers", meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>> LayerMap;
};
