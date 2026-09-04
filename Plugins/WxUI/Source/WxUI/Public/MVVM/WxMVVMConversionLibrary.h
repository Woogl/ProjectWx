// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/Color.h"
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

	/** 뷰모델의 자식 뷰모델처럼 채워졌다 비었다 하는 필드를 그대로 표시 여부로 쓴다. */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Visibility (Object)"))
	static ESlateVisibility Conv_ObjectToSlateVisibility(const UObject* Object, ESlateVisibility TrueVisibility = ESlateVisibility::SelfHitTestInvisible, ESlateVisibility FalseVisibility = ESlateVisibility::Collapsed);

	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Meters (Centimeters)"))
	static double Conv_CentimetersToMeters(double Centimeters, int32 FractionDigits = 1);

	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Tint (Threshold)"))
	static FLinearColor Conv_DoubleToTint(double Value, double Threshold, FLinearColor BelowTint = FLinearColor::Red, FLinearColor NormalTint = FLinearColor::White);

	/**
	 * 없으면 생성한다 — 바인딩이 요청한 어트리뷰트에 대해서만 VM 이 지연 생성된다.
	 * MaxAttribute 미지정 시 Attribute 를 최대값으로 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Get Attribute ViewModel"))
	static UWxViewModel_Attribute* GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute);
};
