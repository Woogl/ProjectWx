// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/SlateWrapperTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxMVVMConversionLibrary.generated.h"

struct FGameplayAttribute;
class UWxViewModel_AbilitySystem;
class UWxViewModel_Attribute;

UCLASS()
class WXUI_API UWxMVVMConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Visibility (GameplayTag)"))
	static ESlateVisibility Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility = ESlateVisibility::SelfHitTestInvisible, ESlateVisibility FalseVisibility = ESlateVisibility::Collapsed);

	/**
	 * 없으면 생성한다 — 바인딩이 요청한 어트리뷰트에 대해서만 VM 이 지연 생성된다.
	 * MaxAttribute 미지정 시 CurrentAttribute 를 최대값으로 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Get Attribute ViewModel"))
	static UWxViewModel_Attribute* GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute CurrentAttribute, FGameplayAttribute MaxAttribute);
};
