// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxUILibrary.generated.h"

class UWidget;
class UWxUIManagerSubsystem;
class UWxPrimaryGameLayout;

/**
 * WxUI 전용 Blueprint Function Library.
 * UI 매니저 및 레이어 제어 유틸리티를 제공한다.
 */
UCLASS()
class WXUI_API UWxUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wx|UI", meta = (WorldContext = "WorldContextObject"))
	static UWxUIManagerSubsystem* GetUIManagerSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Wx|UI", meta = (WorldContext = "WorldContextObject"))
	static UWxPrimaryGameLayout* GetPrimaryGameLayout(const UObject* WorldContextObject);

	/** 위젯의 가장 가까운 부모 ActivatableWidget을 비활성화한다. 패널 닫기 버튼 등에 사용. */
	UFUNCTION(BlueprintCallable, Category = "Wx|UI")
	static void DeactivateOwningActivatable(UWidget* StartingWidget);
};
