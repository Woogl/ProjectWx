// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_AttributeRatio.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTDecorator_AttributeRatio::UWxBTDecorator_AttributeRatio()
{
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
	
	ArithmeticOperation = EArithmeticKeyOperation::LessOrEqual;
	Ratio = 0.5f;
}

FString UWxBTDecorator_AttributeRatio::GetStaticDescription() const
{
	const FString AttributeName = Attribute.IsValid() ? Attribute.GetName() : TEXT("<None>");
	const FString MaxAttributeName = MaxAttribute.IsValid() ? MaxAttribute.GetName() : TEXT("<None>");

	const TCHAR* ComparisonSymbol = TEXT("?");
	switch (ArithmeticOperation)
	{
		case EArithmeticKeyOperation::Less:           ComparisonSymbol = TEXT("<");  break;
		case EArithmeticKeyOperation::LessOrEqual:    ComparisonSymbol = TEXT("<="); break;
		case EArithmeticKeyOperation::Equal:          ComparisonSymbol = TEXT("=="); break;
		case EArithmeticKeyOperation::NotEqual:       ComparisonSymbol = TEXT("!="); break;
		case EArithmeticKeyOperation::GreaterOrEqual: ComparisonSymbol = TEXT(">="); break;
		case EArithmeticKeyOperation::Greater:        ComparisonSymbol = TEXT(">");  break;
		default: break;
	}

	return FString::Printf(TEXT("%s / %s  %s  %.2f"), *AttributeName, *MaxAttributeName, ComparisonSymbol, Ratio);
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

	switch (ArithmeticOperation)
	{
	case EArithmeticKeyOperation::Less:           return CurrentRatio <  Ratio;
	case EArithmeticKeyOperation::LessOrEqual:    return CurrentRatio <= Ratio;
	case EArithmeticKeyOperation::Equal:          return FMath::IsNearlyEqual(CurrentRatio, Ratio);
	case EArithmeticKeyOperation::NotEqual:       return !FMath::IsNearlyEqual(CurrentRatio, Ratio);
	case EArithmeticKeyOperation::GreaterOrEqual: return CurrentRatio >= Ratio;
	case EArithmeticKeyOperation::Greater:        return CurrentRatio >  Ratio;
	default: break;
	}
	return false;
}