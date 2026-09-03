// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxMVVMConversionLibrary.h"

#include "AttributeSet.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}

double UWxMVVMConversionLibrary::Conv_CentimetersToMeters(double Centimeters, int32 FractionDigits)
{
	// 음수면 m 단위 자체가 뭉개지고, 과도하게 크면 Scale 이 발산해 NaN 이 된다.
	const double Scale = FMath::Pow(10.0, static_cast<double>(FMath::Clamp(FractionDigits, 0, 6)));
	return FMath::RoundToDouble(Centimeters * 0.01 * Scale) / Scale;
}

FLinearColor UWxMVVMConversionLibrary::Conv_DoubleToTint(double Value, double Threshold, FLinearColor BelowTint, FLinearColor NormalTint)
{
	return (Value <= Threshold) ? BelowTint : NormalTint;
}

UWxViewModel_Attribute* UWxMVVMConversionLibrary::GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute)
{
	if (!AbilitySystem)
	{
		return nullptr;
	}
	return AbilitySystem->GetOrCreateAttributeViewModel(Attribute, MaxAttribute);
}

