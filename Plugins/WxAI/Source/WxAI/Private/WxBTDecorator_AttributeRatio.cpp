// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_AttributeRatio.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTDecorator_AttributeRatio::UWxBTDecorator_AttributeRatio()
{
	NodeName = TEXT("Compare Attribute Ratio");

	Comparison = EWxAttributeRatioComparison::LessOrEqual;
	Ratio = 0.5f;
}

bool UWxBTDecorator_AttributeRatio::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return false;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC || !Attribute.IsValid() || !MaxAttribute.IsValid())
	{
		return false;
	}

	bool bAttributeFound = false;
	const float CurrentValue = ASC->GetGameplayAttributeValue(Attribute, bAttributeFound);

	bool bMaxFound = false;
	const float MaxValue = ASC->GetGameplayAttributeValue(MaxAttribute, bMaxFound);

	if (!bAttributeFound || !bMaxFound || MaxValue <= 0.0f)
	{
		return false;
	}

	const float CurrentRatio = CurrentValue / MaxValue;

	switch (Comparison)
	{
		case EWxAttributeRatioComparison::Less:           return CurrentRatio <  Ratio;
		case EWxAttributeRatioComparison::LessOrEqual:    return CurrentRatio <= Ratio;
		case EWxAttributeRatioComparison::Equal:          return FMath::IsNearlyEqual(CurrentRatio, Ratio);
		case EWxAttributeRatioComparison::GreaterOrEqual: return CurrentRatio >= Ratio;
		case EWxAttributeRatioComparison::Greater:        return CurrentRatio >  Ratio;
	}
	return false;
}

FString UWxBTDecorator_AttributeRatio::GetStaticDescription() const
{
	const FString AttributeName = Attribute.IsValid() ? Attribute.GetName() : TEXT("<None>");
	const FString MaxAttributeName = MaxAttribute.IsValid() ? MaxAttribute.GetName() : TEXT("<None>");

	const TCHAR* ComparisonSymbol = TEXT("?");
	switch (Comparison)
	{
		case EWxAttributeRatioComparison::Less:           ComparisonSymbol = TEXT("<");  break;
		case EWxAttributeRatioComparison::LessOrEqual:    ComparisonSymbol = TEXT("<="); break;
		case EWxAttributeRatioComparison::Equal:          ComparisonSymbol = TEXT("=="); break;
		case EWxAttributeRatioComparison::GreaterOrEqual: ComparisonSymbol = TEXT(">="); break;
		case EWxAttributeRatioComparison::Greater:        ComparisonSymbol = TEXT(">");  break;
	}

	return FString::Printf(TEXT("%s / %s  %s  %.2f"), *AttributeName, *MaxAttributeName, ComparisonSymbol, Ratio);
}
