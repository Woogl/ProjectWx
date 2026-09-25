// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_FinisherVictim.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
#include "WxAnimNotifySettings.h"
#endif
#include "WxGameplayTags.h"

#if WITH_EDITOR
FLinearColor UWxAnimNotify_FinisherVictim::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AbilityFlowColor;
}
#endif

void UWxAnimNotify_FinisherVictim::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = WxGameplayTags::Event_PlayFinisherVictimMontage;
	Payload.Instigator = Owner;
	Payload.OptionalObject = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}

FString UWxAnimNotify_FinisherVictim::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Victim: %s"), VictimMontage ? *VictimMontage->GetName() : TEXT("None"));
}
