// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_FinisherDamage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
#include "WxAnimNotifySettings.h"
#endif
#include "WxGameplayTags.h"

#if WITH_EDITOR
FLinearColor UWxAnimNotify_FinisherDamage::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AttackColor;
}
#endif

void UWxAnimNotify_FinisherDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = WxGameplayTags::Event_ApplyFinisherDamage;
	Payload.Instigator = Owner;
	Payload.OptionalObject = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}

FString UWxAnimNotify_FinisherDamage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Finisher: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}
