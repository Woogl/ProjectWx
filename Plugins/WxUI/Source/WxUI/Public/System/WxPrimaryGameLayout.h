// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "WxPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;
class UOverlay;

UCLASS(Abstract, meta = (DisableNativeTick))
class WXUI_API UWxPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UWxPrimaryGameLayout();

	UCommonActivatableWidgetStack* GetLayerWidgetStack(FGameplayTag LayerTag) const;

	const TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>>& GetLayerMap() const;

	UCommonActivatableWidget* PushWidgetInstanceToLayerStack(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Layers", meta = (BindWidget))
	TObjectPtr<UOverlay> LayerContainer;

	/** 배열 순서가 z-order다(0 = 최하단). */
	UPROPERTY(EditDefaultsOnly, Category = "UI|Layers", meta = (Categories = "UI.Layer"))
	TArray<FGameplayTag> LayerTags;

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>> LayerMap;
};
