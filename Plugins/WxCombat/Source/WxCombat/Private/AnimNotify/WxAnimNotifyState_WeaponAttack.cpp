// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_WeaponAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "WxGameplayTags.h"

UWxAnimNotifyState_WeaponAttack::UWxAnimNotifyState_WeaponAttack()
{
	HitReactTag = WxGameplayTags::Event_HitReact_Normal;
}

void UWxAnimNotifyState_WeaponAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
	}

	AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner);
	if (!Weapon)
	{
		return;
	}

	if (bUnblockable)
	{
		Weapon->AttackTags.AddTag(WxGameplayTags::Damage_Unblockable);
	}

	if (HitReactTag.IsValid())
	{
		Weapon->AttackTags.AddTag(HitReactTag);
	}

	Weapon->ATKCoeff = ATKCoeff;
}

void UWxAnimNotifyState_WeaponAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
	}

	AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner);
	if (!Weapon)
	{
		return;
	}

	if (bUnblockable)
	{
		Weapon->AttackTags.RemoveTag(WxGameplayTags::Damage_Unblockable);
	}

	if (HitReactTag.IsValid())
	{
		Weapon->AttackTags.RemoveTag(HitReactTag);
	}

	Weapon->ATKCoeff = 1.f;
}

FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Attack");
}
