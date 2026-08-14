// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_RandomWeight.h"

UWxBTDecorator_RandomWeight::UWxBTDecorator_RandomWeight()
{
	NodeName = TEXT("Random Weight");
}

FString UWxBTDecorator_RandomWeight::GetStaticDescription() const
{
	return FString::Printf(TEXT("Weight = %.2f"), Weight);
}

float UWxBTDecorator_RandomWeight::GetWeight() const
{
	return Weight;
}

bool UWxBTDecorator_RandomWeight::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	return true;
}
