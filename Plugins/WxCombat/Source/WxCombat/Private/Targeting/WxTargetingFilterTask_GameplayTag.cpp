// Copyright Woogle. All Rights Reserved.


#include "Targeting/WxTargetingFilterTask_GameplayTag.h"

#include "GameplayTagAssetInterface.h"

bool UWxTargetingFilterTask_GameplayTag::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!TargetActor)
	{
		return false;
	}
	
	const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(TargetActor);
	if (!TagInterface)
	{
		return false;
	}
	
	FGameplayTagContainer OwnedTags;
	TagInterface->GetOwnedGameplayTags(OwnedTags);
	
	return TagRequirements.RequirementsMet(OwnedTags);
}
