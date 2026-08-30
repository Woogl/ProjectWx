// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_BossCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxBossCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void UWxViewModel_BossCharacter::StartObserving(UWorld* World)
{
	if (!World)
	{
		return;
	}

	ObservedWorld = World;
	ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UWxViewModel_BossCharacter::HandleActorSpawned));

	TActorIterator<AWxBossCharacter> ExistingBossIt(World);
	if (ExistingBossIt)
	{
		SetBoss(*ExistingBossIt);
	}
}

void UWxViewModel_BossCharacter::BeginDestroy()
{
	if (UWorld* World = ObservedWorld.Get())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}

	if (AWxBossCharacter* Boss = CurrentBoss.Get())
	{
		Boss->OnEndPlay.RemoveDynamic(this, &UWxViewModel_BossCharacter::HandleBossEndPlay);
		Boss->OnAITargetChanged.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UWxViewModel_BossCharacter::HandleActorSpawned(AActor* SpawnedActor)
{
	if (AWxBossCharacter* Boss = Cast<AWxBossCharacter>(SpawnedActor))
	{
		SetBoss(Boss);
	}
}

void UWxViewModel_BossCharacter::HandleBossEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor == CurrentBoss.Get())
	{
		SetBoss(nullptr);
	}
}

void UWxViewModel_BossCharacter::SetBoss(AWxBossCharacter* Boss)
{
	if (AWxBossCharacter* PreviousBoss = CurrentBoss.Get())
	{
		PreviousBoss->OnEndPlay.RemoveDynamic(this, &UWxViewModel_BossCharacter::HandleBossEndPlay);
		PreviousBoss->OnAITargetChanged.RemoveAll(this);
	}

	CurrentBoss = Boss;

	if (!Boss)
	{
		HandleAITargetChanged(false);
		Deinitialize();
		return;
	}

	Boss->OnEndPlay.AddDynamic(this, &UWxViewModel_BossCharacter::HandleBossEndPlay);
	Boss->OnAITargetChanged.AddUObject(this, &UWxViewModel_BossCharacter::HandleAITargetChanged);
	Initialize(Boss->GetAbilitySystemComponent(), Boss->GetCharacterName(), Boss->GetPortrait());
	HandleAITargetChanged(Boss->HasAITarget());
}

void UWxViewModel_BossCharacter::HandleAITargetChanged(bool bNewHasAITarget)
{
	UE_MVVM_SET_PROPERTY_VALUE(bHasAITarget, bNewHasAITarget);
}

UObject* UWxViewModelResolver_BossCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	UWxViewModel_BossCharacter* ViewModel = NewObject<UWxViewModel_BossCharacter>(PC);
	ViewModel->StartObserving(PC->GetWorld());
	return ViewModel;
}
