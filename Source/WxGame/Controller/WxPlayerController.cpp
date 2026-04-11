// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "Component/WxInteractionComponent.h"
#include "MVVMGameSubsystem.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "Widget/WxActivatableWidget.h"
#include "System/WxUIManagerSubsystem.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "Character/WxPlayerCharacter.h"

namespace WxGlobalViewModelContext
{
	FMVVMViewModelContext PlayerAbilitySystem()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_AbilitySystem::StaticClass();
		Context.ContextName = FName(TEXT("PlayerAbilitySystem"));
		return Context;
	}
}

UInputMappingContext* AWxPlayerController::GetDefaultMappingContext() const
{
	return DefaultMappingContext;
}

const UInputAction* AWxPlayerController::GetMoveAction() const
{
	return MoveAction;
}

const UInputAction* AWxPlayerController::GetLookAction() const
{
	return LookAction;
}

const TArray<FWxInputAbilityBinding>& AWxPlayerController::GetAbilityInputBindings() const
{
	return AbilityInputBindings;
}

UWxInteractionComponent* AWxPlayerController::GetCurrentInteractionTarget() const
{
	return CurrentInteractionTarget.Get();
}

void AWxPlayerController::RegisterInteractionCandidate(UWxInteractionComponent* Component)
{
	if (!Component)
	{
		return;
	}
	InteractionCandidates.AddUnique(Component);
	UpdateCurrentInteractionTarget();
}

void AWxPlayerController::UnregisterInteractionCandidate(UWxInteractionComponent* Component)
{
	if (!Component)
	{
		return;
	}
	for (int32 Index = InteractionCandidates.Num() - 1; Index >= 0; --Index)
	{
		if (InteractionCandidates[Index].Get() == Component)
		{
			InteractionCandidates.RemoveAtSwap(Index);
			break;
		}
	}
	UpdateCurrentInteractionTarget();
}

void AWxPlayerController::UpdateCurrentInteractionTarget()
{
	const APawn* MyPawn = GetPawn();
	UWxInteractionComponent* BestComponent = nullptr;

	if (MyPawn)
	{
		const FVector PawnLocation = MyPawn->GetActorLocation();
		float BestDistanceSq = TNumericLimits<float>::Max();

		for (int32 Index = InteractionCandidates.Num() - 1; Index >= 0; --Index)
		{
			UWxInteractionComponent* Candidate = InteractionCandidates[Index].Get();
			if (!Candidate)
			{
				InteractionCandidates.RemoveAtSwap(Index);
				continue;
			}
			const float DistanceSq = FVector::DistSquared(PawnLocation, Candidate->GetComponentLocation());
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestComponent = Candidate;
			}
		}
	}

	if (CurrentInteractionTarget.Get() != BestComponent)
	{
		CurrentInteractionTarget = BestComponent;
		OnInteractionTargetChanged.Broadcast(BestComponent);
	}
}

void AWxPlayerController::TryInteractCurrent(FGameplayTag InputTag)
{
	UWxInteractionComponent* Target = CurrentInteractionTarget.Get();
	if (!Target)
	{
		return;
	}

	if (HasAuthority())
	{
		Target->TryInteract(GetPawn(), InputTag);
	}
	else
	{
		Server_TryInteract(Target, InputTag);
	}
}

void AWxPlayerController::Server_TryInteract_Implementation(UWxInteractionComponent* TargetComponent, FGameplayTag InputTag)
{
	if (!TargetComponent)
	{
		return;
	}
	TargetComponent->TryInteract(GetPawn(), InputTag);
}

void AWxPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(InPawn))
	{
		if (UAbilitySystemComponent* ASC = WxPlayerCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerAbilitySystemViewModel(ASC);
		}
		// Global View Model 초기화가 먼저 되어야함
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (!IsLocalController())
	{
		return;
	}

	// 원격 클라이언트: Pawn 복제 시 HUD Push 및 ViewModel 초기화
	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(GetPawn()))
	{
		if (UAbilitySystemComponent* ASC = WxPlayerCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerAbilitySystemViewModel(ASC);
		}
		// Global View Model 초기화가 먼저 되어야함
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AWxPlayerController::PushGameHUD(AWxPlayerCharacter* PlayerCharacter)
{
	TSubclassOf<UWxActivatableWidget> HUDClass = PlayerCharacter->GetGameHUDClass();
	if (!HUDClass)
	{
		return;
	}

	UGameInstance* GameInst = GetGameInstance();
	if (!GameInst)
	{
		return;
	}

	UWxUIManagerSubsystem* UIManager = GameInst->GetSubsystem<UWxUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	GameHUD = Cast<UWxActivatableWidget>(UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Game, HUDClass));
}

void AWxPlayerController::InitializePlayerAbilitySystemViewModel(UAbilitySystemComponent* ASC)
{
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
	const FMVVMViewModelContext Context = WxGlobalViewModelContext::PlayerAbilitySystem();

	UWxViewModel_AbilitySystem* ViewModel = NewObject<UWxViewModel_AbilitySystem>(ASC);
	GlobalCollection->AddViewModelInstance(Context, ViewModel);
	ViewModel->Initialize(ASC);

	for (UWxViewModel_Ability* AbilityVM : ViewModel->AbilityViewModels)
	{
		if (!AbilityVM)
		{
			continue;
		}

		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(Spec.Ability);
			if (!WxAbility)
			{
				continue;
			}

			const FGameplayTagContainer& AssetTags = WxAbility->GetAssetTags();
			if (!AssetTags.IsEmpty() && AssetTags.First() == AbilityVM->AbilityTag)
			{
				if (UTexture2D* Icon = WxAbility->AbilityIcon.LoadSynchronous())
				{
					AbilityVM->SetIcon(Icon);
				}
				break;
			}
		}
	}
}
