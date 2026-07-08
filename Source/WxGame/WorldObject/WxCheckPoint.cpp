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

#if WITH_EDITOR
#include "EngineUtils.h"
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/UObjectToken.h"
#endif

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

bool AWxCheckPoint::IsDefaultStart() const
{
	return bIsDefaultStart;
}

#if WITH_EDITOR
void AWxCheckPoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// 복제본은 원본의 PlayerStartTag·bIsDefaultStart 을 그대로 복사받는다. 초기화해 "명백히 미완성" 상태로 두면
	// 디자이너가 새 태그를 지정하도록 강제되고, 방치 시 CheckForErrors 가 태그 미지정으로 잡는다.
	PlayerStartTag = NAME_None;
	bIsDefaultStart = false;
}

void AWxCheckPoint::CheckForErrors()
{
	Super::CheckForErrors();

	// 태그 미지정: 저장·복원 식별자가 없어 부활 지점으로 기능하지 못한다.
	if (PlayerStartTag.IsNone())
	{
		FMessageLog("MapCheck").Error()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::FromString(TEXT("PlayerStartTag 가 지정되지 않았습니다. 고유한 PlayerStartTag 를 지정하세요."))));
	}

	// 같은 레벨의 다른 PlayerStart 와 태그 중복: FindPlayerStartByTag 가 엉뚱한 지점을 먼저 찾을 수 있다.
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Other = *It;
		if (Other != this && !PlayerStartTag.IsNone() && Other->PlayerStartTag == PlayerStartTag)
		{
			FMessageLog("MapCheck").Error()
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("PlayerStartTag '%s' 가 다른 PlayerStart 와 중복됩니다."), *PlayerStartTag.ToString()))));
			break;
		}
	}

	// bIsDefaultStart 다중 지정: 최초 시작지점은 레벨당 1개여야 하며, 다중이면 어느 것이 뽑힐지 불명확하다.
	if (bIsDefaultStart)
	{
		for (TActorIterator<AWxCheckPoint> It(GetWorld()); It; ++It)
		{
			AWxCheckPoint* Other = *It;
			if (Other != this && Other->bIsDefaultStart)
			{
				FMessageLog("MapCheck").Error()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(FText::FromString(TEXT("bIsDefaultStart 가 켜진 체크포인트가 레벨에 둘 이상입니다. 최초 시작지점은 하나여야 합니다."))));
				break;
			}
		}
	}
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

	// 자신을 부활/시작 지점으로 등록한다(메모리). 디스크 영속은 아래 SaveToFile 이 수행하고, ChoosePlayerStart 가 FindPlayerStartByTag 로 이 액터를 찾는다.
	// PlayerStartTag 는 APlayerStart 가 노출하는 식별자로, 디자이너가 인스턴스마다 고유 부여한다(미지정·중복은 CheckForErrors 가 잡는다).
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
