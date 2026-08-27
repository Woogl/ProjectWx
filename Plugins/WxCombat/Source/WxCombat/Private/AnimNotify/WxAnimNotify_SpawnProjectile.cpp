// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnProjectile.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/WxProjectileBase.h"

void UWxAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	// 클라에선 어빌리티 쪽 authority 게이트로 무동작이다.
	if (UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()))
	{
		Ability->SpawnProjectile(ProjectileClass, SpawnSocketName);
	}
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	if (const UClass* TargetClass = ProjectileClass.Get())
	{
		return TargetClass->GetName();
	}
	
	return Super::GetNotifyName_Implementation();
}
