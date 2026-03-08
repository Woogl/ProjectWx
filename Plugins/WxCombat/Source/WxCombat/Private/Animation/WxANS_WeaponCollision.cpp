// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/WxANS_WeaponCollision.h"
#include "Character/WxCharacterBase.h"
#include "Weapon/WxWeaponBase.h"

void UWxANS_WeaponCollision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (AWxCharacterBase* Character = Cast<AWxCharacterBase>(Owner))
		{
			if (AWxWeaponBase* Weapon = Character->GetEquippedWeapon())
			{
				Weapon->SetWeaponCollisionEnabled(true);
			}
		}
	}
}

void UWxANS_WeaponCollision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (AWxCharacterBase* Character = Cast<AWxCharacterBase>(Owner))
		{
			if (AWxWeaponBase* Weapon = Character->GetEquippedWeapon())
			{
				Weapon->SetWeaponCollisionEnabled(false);
			}
		}
	}
}

FString UWxANS_WeaponCollision::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Collision");
}
