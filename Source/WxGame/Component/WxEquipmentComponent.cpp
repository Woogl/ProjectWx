// Copyright Woogle. All Rights Reserved.

#include "Component/WxEquipmentComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UWxEquipmentComponent::UWxEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWxEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxEquipmentComponent, EquippedWeapon);
	DOREPLIFETIME(UWxEquipmentComponent, EquippedItemDef);
}

void UWxEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
}

void UWxEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority() && EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

AWxWeaponBase* UWxEquipmentComponent::GetEquippedWeapon() const
{
	return EquippedWeapon;
}

void UWxEquipmentComponent::SpawnDefaultWeapon()
{
	AActor* OwnerActor = GetOwner();
	if (!WeaponActor || !OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
	if (!OwnerCharacter)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	EquippedWeapon = GetWorld()->SpawnActor<AWxWeaponBase>(WeaponActor, SpawnParams);
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachToCharacter(OwnerCharacter, DefaultWeaponSocket);
	}
}

void UWxEquipmentComponent::EquipItem(const UWxItemDefinition* ItemDef)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (ItemDef && !ItemDef->FindFragment<FWxItemFragment_Equipment>())
	{
		return;
	}

	EquippedItemDef = ItemDef;
	ApplyEquipmentVisuals();
}

void UWxEquipmentComponent::OnRep_EquippedWeapon()
{
	// 늦게 도착한 EquippedWeapon이 기존 EquippedItemDef를 따라 시각에 반영되도록 재적용.
	ApplyEquipmentVisuals();
}

void UWxEquipmentComponent::OnRep_EquippedItemDef()
{
	ApplyEquipmentVisuals();
}

void UWxEquipmentComponent::ApplyEquipmentVisuals()
{
	if (!EquippedWeapon)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	// EquippedItemDef가 없으면 CDO 기본 메시 + 기본 소켓으로 장착 (장착 해제 상태).
	FName Socket = DefaultWeaponSocket;
	USkeletalMesh* MeshAsset = nullptr;

	if (EquippedItemDef)
	{
		if (const FWxItemFragment_Equipment* Fragment = EquippedItemDef->FindFragment<FWxItemFragment_Equipment>())
		{
			Socket = Fragment->AttachSocket;
			MeshAsset = Fragment->SkeletalMesh;
		}
	}

	EquippedWeapon->SetVisualMesh(MeshAsset);
	EquippedWeapon->AttachToCharacter(OwnerCharacter, Socket);
}
