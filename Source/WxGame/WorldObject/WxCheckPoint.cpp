// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxCheckPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Interaction/WxInteractionComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "System/WxSpawnerSubsystem.h"
#include "WxSaveGameSubsystem.h"

AWxCheckPoint::AWxCheckPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// OnInteracted 는 서버 Multicast RPC 로 모든 피어에서 fire 되므로 액터 복제가 필요하다(과거 AWxGimmick 가 켜주던 설정을 직접 유지).
	bReplicates = true;

	// 루트는 APlayerStart(ANavigationObjectBase)의 CapsuleComponent 다. NoCollision 프로파일이라 플레이어를 막지 않는다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxCheckPoint::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxCheckPoint::HandleInteracted);
}

void AWxCheckPoint::HandleInteracted(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// 자신을 활성 부활 지점으로 등록한다. 이후 SaveSlot 이 이 태그를 디스크에 영속하고, ChoosePlayerStart 가 FindPlayerStart 로 이 액터를 찾는다.
	// PlayerStartTag 는 APlayerStart 가 노출하는 식별자로, 디자이너가 인스턴스마다 고유 부여한다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWxSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWxSaveGameSubsystem>())
		{
			SaveSubsystem->SetActiveCheckpoint(PlayerStartTag);
		}
	}

	if (HealEffect)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor))
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffect, 1.0f, Context);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 에스트병 등 충전형 소비 아이템의 충전을 가득 채운다(다크소울 모닥불 방식). 충전형이 아닌 아이템은 내부에서 무시된다.
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(InstigatorActor))
	{
		for (UWxItemInstance* Item : Inventory->GetAllItems())
		{
			Inventory->RefillItemCharges(Item);
		}
	}

	if (UWxSpawnerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWxSpawnerSubsystem>())
	{
		Subsystem->RespawnAutoSpawners();
	}
}
