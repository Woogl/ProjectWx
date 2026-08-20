// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widget/WxGamePopup.h"
#include "WxUILibrary.generated.h"

class UCommonActivatableWidget;
class UWidget;
class UWxUIManagerSubsystem;
class UWxPrimaryGameLayout;

DECLARE_DYNAMIC_DELEGATE_OneParam(FWxPopupResultDynamicDelegate, EWxPopupResult, Result);

UENUM(BlueprintType)
enum class EWxPopupButtonLayout : uint8
{
	Ok,
	OkCancel,
	YesNo,
	YesNoCancel
};

UCLASS()
class WXUI_API UWxUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wx|UI", meta = (WorldContext = "WorldContextObject"))
	static UWxUIManagerSubsystem* GetUIManagerSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Wx|UI", meta = (WorldContext = "WorldContextObject"))
	static UWxPrimaryGameLayout* GetPrimaryGameLayout(const UObject* WorldContextObject);

	/** 소프트 위젯 클래스를 동기 로드해 push 한다. 클래스 미지정·로드 실패면 아무 것도 하지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|UI", meta = (WorldContext = "WorldContextObject"))
	static UCommonActivatableWidget* PushSoftContentToLayer(const UObject* WorldContextObject, UPARAM(meta = (Categories = "UI.Layer"))FGameplayTag LayerTag, TSoftClassPtr<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Wx|UI")
	static void DeactivateOwningActivatable(UWidget* StartingWidget);

	UFUNCTION(BlueprintCallable, Category = "Wx|UI", meta = (WorldContext = "WorldContextObject"))
	static void DeactivateWidgetsInLayer(const UObject* WorldContextObject, UPARAM(meta = (Categories = "UI.Layer"))FGameplayTag LayerTag);

	UFUNCTION(BlueprintCallable, Category = "Wx|UI|Popup", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "OnResult"))
	static void ShowConfirmationPopup(const UObject* WorldContextObject, EWxPopupButtonLayout Buttons, FText Header, FText Body, const FWxPopupResultDynamicDelegate& OnResult);
};
