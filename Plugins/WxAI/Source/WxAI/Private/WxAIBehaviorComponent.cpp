// Copyright Woogle. All Rights Reserved.

#include "WxAIBehaviorComponent.h"
#include "WxAIModule.h"
#include "GameFramework/Pawn.h"

void UWxAIBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner<APawn>())
	{
		UE_LOG(LogWxAI, Warning, TEXT("%s: WxAIBehaviorComponent 는 Pawn 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
	}
}

UBehaviorTree* UWxAIBehaviorComponent::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

float UWxAIBehaviorComponent::GetSightRadius() const
{
	return SightRadius;
}

float UWxAIBehaviorComponent::GetSightAngle() const
{
	return SightAngle;
}

float UWxAIBehaviorComponent::GetHearingRadius() const
{
	return HearingRadius;
}
