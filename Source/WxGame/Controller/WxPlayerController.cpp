// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "MVVMGameSubsystem.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "Widget/WxActivatableWidget.h"
#include "System/WxUIManagerSubsystem.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "Character/WxPlayerCharacter.h"

namespace WxGlobalViewModelContext
{
	FMVVMViewModelContext PlayerHP()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_Attribute::StaticClass();
		Context.ContextName = FName(TEXT("PlayerHP"));
		return Context;
	}

	FMVVMViewModelContext PlayerMP()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_Attribute::StaticClass();
		Context.ContextName = FName(TEXT("PlayerMP"));
		return Context;
	}
	
	FMVVMViewModelContext PlayerUP()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_Attribute::StaticClass();
		Context.ContextName = FName(TEXT("PlayerUP"));
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

	if (!IsLocalController())
	{
		return;
	}

	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(GetPawn()))
	{
		if (TSubclassOf<UWxActivatableWidget> GameHUDClass = WxPlayerCharacter->GetGameHUDClass())
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
			InitializePlayerHPViewModel(ASC);
			InitializePlayerMPViewModel(ASC);
			InitializePlayerUPViewModel(ASC);
			InitializePlayerAbilityViewModels(ASC);
		}
	}
}

void AWxPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (!IsLocalController())
	{
		return;
	}
	
	// 원격 클라이언트: Pawn 복제 시 ViewModel 초기화
	if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(GetPawn()))
	{
		if (UAbilitySystemComponent* ASC = WxCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerHPViewModel(ASC);
			InitializePlayerMPViewModel(ASC);
			InitializePlayerUPViewModel(ASC);
			InitializePlayerAbilityViewModels(ASC);
		}
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (UGameInstance* GameInst = GetGameInstance())
	{
		if (UMVVMGameSubsystem* MVVMGameSubsystem = GameInst->GetSubsystem<UMVVMGameSubsystem>())
		{
			MVVMGameSubsystem->GetViewModelCollection()->RemoveViewModel(WxGlobalViewModelContext::PlayerHP());
			MVVMGameSubsystem->GetViewModelCollection()->RemoveViewModel(WxGlobalViewModelContext::PlayerMP());
			MVVMGameSubsystem->GetViewModelCollection()->RemoveViewModel(WxGlobalViewModelContext::PlayerUP());
		}
	}
}

void AWxPlayerController::InitializePlayerHPViewModel(UAbilitySystemComponent* ASC)
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
	const FMVVMViewModelContext Context = WxGlobalViewModelContext::PlayerHP();

	UWxViewModel_Attribute* ViewModel = ViewModel = NewObject<UWxViewModel_Attribute>(ASC);
	GlobalCollection->AddViewModelInstance(Context, ViewModel);
	ViewModel->Initialize(ASC, UWxCombatAttributeSet::GetHPAttribute(), UWxCombatAttributeSet::GetMaxHPAttribute());
}

void AWxPlayerController::InitializePlayerMPViewModel(UAbilitySystemComponent* ASC)
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
	const FMVVMViewModelContext Context = WxGlobalViewModelContext::PlayerMP();

	UWxViewModel_Attribute* ViewModel = ViewModel = NewObject<UWxViewModel_Attribute>(ASC);
	GlobalCollection->AddViewModelInstance(Context, ViewModel);
	ViewModel->Initialize(ASC, UWxCombatAttributeSet::GetMPAttribute(), UWxCombatAttributeSet::GetMaxMPAttribute());
}

void AWxPlayerController::InitializePlayerUPViewModel(UAbilitySystemComponent* ASC)
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
	const FMVVMViewModelContext Context = WxGlobalViewModelContext::PlayerUP();

	UWxViewModel_Attribute* ViewModel = NewObject<UWxViewModel_Attribute>(ASC);
	GlobalCollection->AddViewModelInstance(Context, ViewModel);
	ViewModel->Initialize(ASC, UWxCombatAttributeSet::GetUPAttribute(), UWxCombatAttributeSet::GetMaxUPAttribute());
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
		if (!AbilityCDO)
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

		UWxViewModel_Ability* ViewModel = ViewModel = NewObject<UWxViewModel_Ability>(ASC);
		GlobalCollection->AddViewModelInstance(Context, ViewModel);
		ViewModel->SetIcon(AbilityCDO->AbilityIcon.LoadSynchronous());
		ViewModel->Initialize(ASC, AbilityCDO);
	}
}
