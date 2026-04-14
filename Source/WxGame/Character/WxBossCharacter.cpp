// Copyright Woogle. All Rights Reserved.

#include "Character/WxBossCharacter.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MVVMGameSubsystem.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

namespace WxGlobalViewModelContext
{
	FMVVMViewModelContext BossAbilitySystem()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_AbilitySystem::StaticClass();
		Context.ContextName = FName(TEXT("BossAbilitySystem"));
		return Context;
	}
}

static const FName BBKey_TargetActor = TEXT("TargetActor");

void AWxBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			const FBlackboard::FKey KeyID = BB->GetKeyID(BBKey_TargetActor);
			if (KeyID != FBlackboard::InvalidKey)
			{
				BB->RegisterObserver(KeyID, this, FOnBlackboardChangeNotification::CreateUObject(this, &AWxBossCharacter::HandleBlackboardValueChanged));
			}
		}
	}
}

void AWxBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterBossAbilitySystemViewModel();

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
	const UObject* TargetActor = BlackboardComp.GetValueAsObject(BBKey_TargetActor);

	if (TargetActor)
	{
		RegisterBossAbilitySystemViewModel();
	}
	else
	{
		UnregisterBossAbilitySystemViewModel();
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

void AWxBossCharacter::RegisterBossAbilitySystemViewModel()
{
	if (BossAbilitySystemViewModel)
	{
		return;
	}

	UGameInstance* GameInst = GetGameInstance();
	if (!GameInst)
	{
		return;
	}

	UMVVMGameSubsystem* MVVMGameSubsystem = GameInst->GetSubsystem<UMVVMGameSubsystem>();
	if (!MVVMGameSubsystem)
	{
		return;
	}

	UMVVMViewModelCollectionObject* GlobalCollection = MVVMGameSubsystem->GetViewModelCollection();
	const FMVVMViewModelContext Context = WxGlobalViewModelContext::BossAbilitySystem();

	BossAbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(AbilitySystemComponent);
	GlobalCollection->AddViewModelInstance(Context, BossAbilitySystemViewModel);
	BossAbilitySystemViewModel->Initialize(AbilitySystemComponent);
}

void AWxBossCharacter::UnregisterBossAbilitySystemViewModel()
{
	if (!BossAbilitySystemViewModel)
	{
		return;
	}

	UGameInstance* GameInst = GetGameInstance();
	if (!GameInst)
	{
		return;
	}

	UMVVMGameSubsystem* MVVMGameSubsystem = GameInst->GetSubsystem<UMVVMGameSubsystem>();
	if (!MVVMGameSubsystem)
	{
		return;
	}

	UMVVMViewModelCollectionObject* GlobalCollection = MVVMGameSubsystem->GetViewModelCollection();
	const FMVVMViewModelContext Context = WxGlobalViewModelContext::BossAbilitySystem();

	GlobalCollection->RemoveViewModel(Context);
	BossAbilitySystemViewModel = nullptr;
}
