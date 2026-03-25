// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_PerfectGuard.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

void UWxAnimNotifyState_PerfectGuard::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			ASC->AddLooseGameplayTag(WxGameplayTags::ANS_PerfectGuard);
		}
	}
}

void UWxAnimNotifyState_PerfectGuard::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_PerfectGuard);
		}
	}
}

FString UWxAnimNotifyState_PerfectGuard::GetNotifyName_Implementation() const
{
	return TEXT("Perfect Guard");
}
