// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "MVVMGameSubsystem.h"
#include "WxViewModel_Health.h"

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

const UWxInputConfig* AWxPlayerController::GetInputConfig() const
{
	return InputConfig;
}

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(InPawn))
	{
		if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(WxCharacter->GetAbilitySystemComponent()))
		{
			if (ASC->IsInitialized())
			{
				HandleAbilitySystemInitialized(ASC);
			}
			else
			{
				ASC->OnInitialized.AddUObject(this, &AWxPlayerController::HandleAbilitySystemInitialized);
			}
		}
	}
}

void AWxPlayerController::OnUnPossess()
{
	if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(GetPawn()))
	{
		if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(WxCharacter->GetAbilitySystemComponent()))
		{
			ASC->OnInitialized.RemoveAll(this);
		}
	}

	Super::OnUnPossess();
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

void AWxPlayerController::HandleAbilitySystemInitialized(UAbilitySystemComponent* ASC)
{
	UMVVMGameSubsystem* MVVMGameSubsystem = GetGameInstance()->GetSubsystem<UMVVMGameSubsystem>();
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

	ViewModel->InitializeWithASC(ASC);
}
