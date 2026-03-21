// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "MVVMGameSubsystem.h"
#include "MVVM/WxViewModel_Health.h"
#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "Widget/WxActivatableWidget.h"
#include "System/WxUIManagerSubsystem.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/WxAttributeSet.h"

namespace
{
	FMVVMViewModelContext GetPlayerHealthViewModelContext()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_Health::StaticClass();
		Context.ContextName = FName(TEXT("PlayerHealth"));
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

void AWxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && GameHUDClass)
	{
		if (UGameInstance* GameInst = GetGameInstance())
		{
			if (UWxUIManagerSubsystem* UIManager = GameInst->GetSubsystem<UWxUIManagerSubsystem>())
			{
				UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Game, GameHUDClass);
			}
		}
	}
}

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (IsLocalController())
	{
		if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(InPawn))
		{
			if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(WxCharacter->GetAbilitySystemComponent()))
			{
				InitializePlayerHealthViewModel(ASC);
				InitializePlayerAbilityViewModels(ASC);
			}
		}
	}
}

void AWxPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	// 원격 클라이언트: Pawn 복제 시 ViewModel 초기화
	if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(GetPawn()))
	{
		if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(WxCharacter->GetAbilitySystemComponent()))
		{
			InitializePlayerHealthViewModel(ASC);
			InitializePlayerAbilityViewModels(ASC);
		}
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInst = GetGameInstance())
	{
		if (UMVVMGameSubsystem* MVVMGameSubsystem = GameInst->GetSubsystem<UMVVMGameSubsystem>())
		{
			MVVMGameSubsystem->GetViewModelCollection()->RemoveViewModel(GetPlayerHealthViewModelContext());
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AWxPlayerController::InitializePlayerHealthViewModel(UAbilitySystemComponent* ASC)
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
	const FMVVMViewModelContext Context = GetPlayerHealthViewModelContext();

	UWxViewModel_Health* ViewModel = Cast<UWxViewModel_Health>(GlobalCollection->FindViewModelInstance(Context));
	if (!ViewModel)
	{
		ViewModel = NewObject<UWxViewModel_Health>(this);
		GlobalCollection->AddViewModelInstance(Context, ViewModel);
	}

	ViewModel->Initialize(ASC, UWxAttributeSet::GetHPAttribute(), UWxAttributeSet::GetMaxHPAttribute());
}

void AWxPlayerController::InitializePlayerAbilityViewModels(UAbilitySystemComponent* ASC)
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

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UWxAbility* AbilityCDO = Cast<UWxAbility>(Spec.Ability);
		if (!AbilityCDO || !AbilityCDO->CooldownTag.IsValid())
		{
			continue;
		}

		const FGameplayTagContainer& AssetTags = AbilityCDO->GetAssetTags();
		if (AssetTags.IsEmpty())
		{
			continue;
		}

		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_Ability::StaticClass();
		Context.ContextName = AssetTags.First().GetTagName();

		UWxViewModel_Ability* ViewModel = Cast<UWxViewModel_Ability>(GlobalCollection->FindViewModelInstance(Context));
		if (!ViewModel)
		{
			ViewModel = NewObject<UWxViewModel_Ability>(this);
			GlobalCollection->AddViewModelInstance(Context, ViewModel);
		}

		ViewModel->Initialize(ASC, AbilityCDO);
	}
}
