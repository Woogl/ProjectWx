// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxItemUseComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AnimNotify/WxAnimNotify_UseItem.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "WxGameplayTags.h"

bool UWxItemUseComponent::CanUseItem(const UWxItemDefinition* ItemDefinition) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const UWxInventoryComponent* Inventory = UWxInventoryComponent::FindInventory(Pawn);
	return Inventory && Inventory->CanUseItemByDef(ItemDefinition);
}

void UWxItemUseComponent::BeginUseItem(UWxItemDefinition* ItemDefinition)
{
	PendingItemDefinition = ItemDefinition;
}

void UWxItemUseComponent::EndUseItem(const UWxItemDefinition* ItemDefinition)
{
	if (PendingItemDefinition == ItemDefinition)
	{
		PendingItemDefinition = nullptr;
	}
}

void UWxItemUseComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	AbilitySystemComponent = ASC;
	UseItemEventHandle = ASC->GenericGameplayEventCallbacks
		.FindOrAdd(WxGameplayTags::Event_UseItem)
		.AddUObject(this, &UWxItemUseComponent::HandleUseItemEvent);
}

void UWxItemUseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(WxGameplayTags::Event_UseItem))
		{
			EventDelegate->Remove(UseItemEventHandle);
		}
	}

	PendingItemDefinition = nullptr;
	AbilitySystemComponent.Reset();
	UseItemEventHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UWxItemUseComponent::HandleUseItemEvent(const FGameplayEventData* Payload)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Payload || !PendingItemDefinition)
	{
		return;
	}

	const UWxAnimNotify_UseItem* UseItemNotify = Cast<UWxAnimNotify_UseItem>(Payload->OptionalObject.Get());
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Payload->OptionalObject2.Get());
	if (!UseItemNotify || !Mesh || Mesh->GetOwner() != Owner)
	{
		return;
	}

	UWxItemDefinition* ItemDefinition = PendingItemDefinition;
	PendingItemDefinition = nullptr;

	if (UWxInventoryComponent* Inventory = UWxInventoryComponent::FindInventory(Cast<APawn>(Owner)))
	{
		Inventory->UseItemByDef(ItemDefinition);
	}
}
