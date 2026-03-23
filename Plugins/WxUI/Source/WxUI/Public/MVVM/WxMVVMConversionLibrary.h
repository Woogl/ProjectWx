// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/SlateWrapperTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxMVVMConversionLibrary.generated.h"

/**
 * WxUI 전용 Blueprint Function Library.
 * MVVM 컨버전 함수 등 UI 바인딩에 필요한 유틸리티를 제공한다.
 */
UCLASS()
class WXUI_API UWxMVVMConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** FGameplayTagContainer에 특정 태그가 포함되어 있는지 판정한다. */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Bool (GameplayTag)"))
	static bool Conv_GameplayTagToBool(const FGameplayTagContainer& TagContainer, FGameplayTag Tag);

	/** 특정 태그 보유 여부에 따라 Visibility를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Visibility (GameplayTag)"))
	static ESlateVisibility Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility = ESlateVisibility::SelfHitTestInvisible, ESlateVisibility FalseVisibility = ESlateVisibility::Collapsed);
};
