// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxCheckPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Interaction/WxInteractionComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "System/WxSpawnerLibrary.h"
#include "WxPersistenceGameSubsystem.h"

AWxCheckPoint::AWxCheckPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 원격 클라가 이 액터의 InteractionComponent 를 상호작용 TargetData(PackageMap)로 서버에 참조 전달하므로, 컴포넌트가 net-addressable 하도록 액터 복제를 유지한다.
	bReplicates = true;

	// 루트는 APlayerStart(ANavigationObjectBase)의 CapsuleComponent 다. NoCollision 프로파일이라 플레이어를 막지 않는다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
	InteractionComponent->SetHighlightTarget(MeshComponent);
}

#if WITH_EDITOR
// 에디터 전용 GetActorGuid() 를 PlayerStartTag 로 1회 베이킹한다.
// ActorGuid 는 에디터에서 액터별 안정·고유하고, 부여된 태그는 레벨에 직렬화돼 런타임/세션 간 불변이다.
void AWxCheckPoint::PostActorCreated()
{
	Super::PostActorCreated();

	// 신규 배치 시 태그가 비어 있으면 GUID 부여. 디자이너가 지정한 태그는 보존.
	if (PlayerStartTag.IsNone())
	{
		PlayerStartTag = FName(GetActorGuid().ToString());
	}
}

void AWxCheckPoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// 복제 시 엔진이 새 ActorGuid 를 부여하지만 PlayerStartTag 문자열은 원본값이 복사된다.
	// 새 ActorGuid 로 재부여해 원본과의 태그 충돌을 막는다.
	PlayerStartTag = FName(GetActorGuid().ToString());
}
#endif

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

	UWxPersistenceGameSubsystem* SaveSubsystem = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SaveSubsystem = GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>();
	}

	// 자신을 부활/시작 지점으로 등록한다(메모리). 디스크 영속은 아래 SaveToFile 이 수행하고, ChoosePlayerStart 가 FindPlayerStart 로 이 액터를 찾는다.
	// PlayerStartTag 는 APlayerStart 가 노출하는 식별자로, 디자이너가 인스턴스마다 고유 부여한다.
	if (SaveSubsystem)
	{
		SaveSubsystem->SetPlayerStartTag(PlayerStartTag);
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

	UWxSpawnerLibrary::TryRespawnAll(this);

	// 갱신된 PlayerStartTag + 리셋된 월드 상태를 활성 슬롯에 저장한다. 과거 BP OnInteracted 의 저장 호출을 C++ 로 이관 —
	// SetPlayerStartTag 이후 실행돼 순서가 보장되고, HasAuthority 게이트 안이라 서버 전용으로 저장된다(슬롯 정체성은 활성 SaveGame 이 보유).
	if (SaveSubsystem)
	{
		SaveSubsystem->SaveToFile();
	}
}
