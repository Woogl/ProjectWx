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
	// 조건이 아니라 RandomChoice 가중치 데이터를 운반하는 Decorator 이므로 실행을 절대 막지 않는다.
	return true;
}
