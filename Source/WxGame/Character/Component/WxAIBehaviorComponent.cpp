// Copyright Woogle. All Rights Reserved.

#include "Character/Component/WxAIBehaviorComponent.h"
#include "Character/WxCharacterBase.h"
#include "WxGame.h"

void UWxAIBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner<AWxCharacterBase>())
	{
		UE_LOG(LogWxGame, Warning, TEXT("%s: WxAIBehaviorComponent 는 WxCharacterBase 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
	}
}

UBehaviorTree* UWxAIBehaviorComponent::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}
