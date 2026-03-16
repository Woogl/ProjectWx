// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "MVVMGameSubsystem.h"
#include "WxViewModel_Health.h"
#include "WxActivatableWidget.h"
#include "WxUIManagerSubsystem.h"
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

	// 리슨 서버: 서버이면서 로컬 컨트롤러인 경우 ViewModel 초기화
	if (IsLocalController())
	{
		if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(InPawn))
		{
			if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(WxCharacter->GetAbilitySystemComponent()))
			{
				InitializePlayerHealthViewModel(ASC);
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

	ViewModel->InitializeWithASC(ASC, UWxAttributeSet::GetHPAttribute(), UWxAttributeSet::GetMaxHPAttribute());
}
