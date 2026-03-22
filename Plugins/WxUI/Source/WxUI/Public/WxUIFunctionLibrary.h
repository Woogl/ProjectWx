// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxUIFunctionLibrary.generated.h"

/**
 * WxUI 전용 Blueprint Function Library.
 * MVVM 컨버전 함수 등 UI 바인딩에 필요한 유틸리티를 제공한다.
 */
UCLASS()
class WXUI_API UWxUIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** FGameplayTagContainer에 특정 태그가 포함되어 있는지 판정한다. MVVM 컨버전 함수로 사용. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wx|UI")
	static bool HasGameplayTag(const FGameplayTagContainer& TagContainer, FGameplayTag Tag);
};
