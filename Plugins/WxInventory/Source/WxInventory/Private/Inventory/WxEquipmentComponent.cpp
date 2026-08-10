// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxEquipmentComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Net/UnrealNetwork.h"

UWxEquipmentComponent::UWxEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWxEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxEquipmentComponent, EquippedItemDef);
}

void UWxEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RemoveActiveEquipEffects();
	}

	Super::EndPlay(EndPlayReason);
}

void UWxEquipmentComponent::EquipItem(const UWxItemDefinition* ItemDef)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	const UWxItemFragment_Equippable* NewFragment = ItemDef ? ItemDef->FindFragmentByClass<UWxItemFragment_Equippable>() : nullptr;
	if (ItemDef && !NewFragment)
	{
		return;
	}

	// 이전 장비의 GE 를 먼저 제거한 뒤 신규 장비를 적용한다 — 스왑 시 효과 누적/공백을 모두 방지.
	RemoveActiveEquipEffects();

	EquippedItemDef = ItemDef;
	BroadcastEquipVisual();

	if (NewFragment)
	{
		ApplyEquipEffects(ItemDef, NewFragment->EquipEffects);
	}
}

void UWxEquipmentComponent::OnRep_EquippedItemDef()
{
	BroadcastEquipVisual();
}

void UWxEquipmentComponent::BroadcastEquipVisual()
{
	USkeletalMesh* MeshAsset = nullptr;
	FName Socket = NAME_None;

	if (EquippedItemDef)
	{
		if (const UWxItemFragment_Equippable* Fragment = EquippedItemDef->FindFragmentByClass<UWxItemFragment_Equippable>())
		{
			MeshAsset = Fragment->SkeletalMesh;
			Socket = Fragment->AttachSocket;
		}
	}

	OnEquipVisualChanged.Broadcast(MeshAsset, Socket);
}

void UWxEquipmentComponent::ApplyEquipEffects(const UWxItemDefinition* SourceDef, const TArray<TSubclassOf<UGameplayEffect>>& Effects)
{
	if (Effects.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* ASC = ResolveOwnerASC();
	if (!ASC)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Effects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(SourceDef);

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (!Spec.IsValid())
		{
			continue;
		}

		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (Handle.IsValid())
		{
			ActiveEquipEffectHandles.Add(Handle);
		}
	}
}

void UWxEquipmentComponent::RemoveActiveEquipEffects()
{
	if (ActiveEquipEffectHandles.IsEmpty())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = ResolveOwnerASC())
	{
		for (const FActiveGameplayEffectHandle& Handle : ActiveEquipEffectHandles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}

	ActiveEquipEffectHandles.Reset();
}

UAbilitySystemComponent* UWxEquipmentComponent::ResolveOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
