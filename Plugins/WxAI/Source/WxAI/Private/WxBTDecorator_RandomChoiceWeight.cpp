// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_RandomChoiceWeight.h"

UWxBTDecorator_RandomChoiceWeight::UWxBTDecorator_RandomChoiceWeight()
{
	NodeName = TEXT("Random Choice Weight");
}

FString UWxBTDecorator_RandomChoiceWeight::GetStaticDescription() const
{
	return FString::Printf(TEXT("Weight = %.2f"), Weight);
}

bool UWxBTDecorator_RandomChoiceWeight::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// 조건이 아니라 RandomChoice 가중치 데이터를 운반하는 Decorator 이므로 실행을 절대 막지 않는다.
	return true;
}
