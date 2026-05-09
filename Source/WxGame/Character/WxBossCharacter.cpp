// Copyright Woogle. All Rights Reserved.

#include "Character/WxBossCharacter.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "WxAIBlackboardKeys.h"
#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UWxViewModel_AbilitySystem* GetBossAbilitySystemViewModel(const UObject* WorldContext)
	{
		const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
		APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
		ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		if (!LocalPlayer)
		{
			return nullptr;
		}

		UWxGlobalViewModelSubsystem* Subsystem = LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>();
		return Subsystem ? Subsystem->GetBossAbilitySystemViewModel() : nullptr;
	}
}

void AWxBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			const FBlackboard::FKey KeyID = BB->GetKeyID(WxAIBlackboardKeys::TargetActor);
			if (KeyID != FBlackboard::InvalidKey)
			{
				BB->RegisterObserver(KeyID, this, FOnBlackboardChangeNotification::CreateUObject(this, &AWxBossCharacter::HandleBlackboardValueChanged));
			}
		}
	}
}

void AWxBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateBossAbilitySystemViewModel();

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->UnregisterObserversFrom(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

EBlackboardNotificationResult AWxBossCharacter::HandleBlackboardValueChanged(const UBlackboardComponent& BlackboardComp, FBlackboard::FKey ChangedKeyID)
{
	const UObject* TargetActor = BlackboardComp.GetValueAsObject(WxAIBlackboardKeys::TargetActor);

	if (TargetActor)
	{
		ActivateBossAbilitySystemViewModel();
	}
	else
	{
		DeactivateBossAbilitySystemViewModel();
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

void AWxBossCharacter::ActivateBossAbilitySystemViewModel()
{
	if (UWxViewModel_AbilitySystem* VM = GetBossAbilitySystemViewModel(this))
	{
		VM->Initialize(AbilitySystemComponent);
	}
}

void AWxBossCharacter::DeactivateBossAbilitySystemViewModel()
{
	if (UWxViewModel_AbilitySystem* VM = GetBossAbilitySystemViewModel(this))
	{
		VM->Deinitialize();
	}
}
