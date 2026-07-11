// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "WxPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;

UCLASS(Abstract)
class WXUI_API UWxPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetStack* GetLayerWidgetStack(FGameplayTag LayerTag) const;

	UCommonActivatableWidget* PushWidgetToLayerStack(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 위젯을 스택에 push 하되, 활성화되기 전에 InitInstanceFunc 로 인스턴스를 초기화한다. */
	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass, TFunctionRef<void(ActivatableWidgetT&)> InitInstanceFunc)
	{
		UCommonActivatableWidgetStack* Stack = GetLayerWidgetStack(LayerTag);
		if (!Stack)
		{
			return nullptr;
		}
		return Stack->AddWidget<ActivatableWidgetT>(WidgetClass, InitInstanceFunc);
	}

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
