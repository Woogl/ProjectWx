// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileManagerComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AnimNotify/WxAnimNotify_SpawnProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapon/WxProjectileBase.h"
#include "WxGameplayTags.h"

void UWxProjectileManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	AbilitySystemComponent = ASC;
	SpawnProjectileEventHandle = ASC->GenericGameplayEventCallbacks
		.FindOrAdd(WxGameplayTags::Event_SpawnProjectile)
		.AddUObject(this, &UWxProjectileManagerComponent::HandleSpawnProjectileEvent);
}

void UWxProjectileManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(WxGameplayTags::Event_SpawnProjectile))
		{
			EventDelegate->Remove(SpawnProjectileEventHandle);
		}
	}

	AbilitySystemComponent.Reset();
	SpawnProjectileEventHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UWxProjectileManagerComponent::HandleSpawnProjectileEvent(const FGameplayEventData* Payload)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Payload)
	{
		return;
	}

	const UWxAnimNotify_SpawnProjectile* ProjectileNotify = Cast<UWxAnimNotify_SpawnProjectile>(Payload->OptionalObject.Get());
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Payload->OptionalObject2.Get());
	if (!ProjectileNotify || !Mesh || Mesh->GetOwner() != Owner || !ProjectileNotify->GetProjectileClass())
	{
		return;
	}

	const FVector SpawnLocation = Mesh->GetSocketLocation(ProjectileNotify->GetSpawnSocketName());
	const FTransform SpawnTransform(Owner->GetActorRotation(), SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Owner->GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileNotify->GetProjectileClass(), SpawnTransform, SpawnParams);
}
