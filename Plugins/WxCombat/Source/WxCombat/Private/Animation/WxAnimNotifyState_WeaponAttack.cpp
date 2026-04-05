// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_WeaponAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "WxGameplayTags.h"

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

	if (!AttackTags.IsEmpty())
	{
		if (AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner))
		{
			Weapon->AttackTags.AppendTags(AttackTags);
		}
	}
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

	if (!AttackTags.IsEmpty())
	{
		if (AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner))
		{
			Weapon->AttackTags.RemoveTags(AttackTags);
		}
	}
}

FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Attack");
}
