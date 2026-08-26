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

	UCommonActivatableWidget* PushWidgetToLayerStack(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 위젯을 스택에 push 하되, 활성화되기 전에 InitInstanceFunc 로 인스턴스를 초기화한다. */
	// 헤더 정의는 코딩 규칙 6 의 예외다 — 템플릿이라 cpp 로 내릴 수 없다.
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
	TObjectPtr<UOverlay> LayerContainer;

	/** 배열 순서가 z-order다(0 = 최하단). */
	UPROPERTY(EditDefaultsOnly, Category = "UI|Layers", meta = (Categories = "UI.Layer"))
	TArray<FGameplayTag> LayerTags;

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>> LayerMap;
};
