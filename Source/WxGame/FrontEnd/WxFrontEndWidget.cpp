// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxFrontEndWidget.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "FrontEnd/WxGameFlowSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "System/WxUIManagerSubsystem.h"

#define LOCTEXT_NAMESPACE "WxFrontEnd"

void UWxFrontEndWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	NewGame->OnClicked().AddUObject(this, &ThisClass::HandleNewGame);
	QuitGame->OnClicked().AddUObject(this, &ThisClass::HandleQuitGame);
	TemplatePlayerButton->OnClicked().AddUObject(this, &ThisClass::HandleSelectCharacter, 0);
	BP_HGTest->OnClicked().AddUObject(this, &ThisClass::HandleSelectCharacter, 1);
	CombatButton->OnClicked().AddUObject(this, &ThisClass::HandleSelectDestination, 0);
	OpenWorldButton->OnClicked().AddUObject(this, &ThisClass::HandleSelectDestination, 1);
}

void UWxFrontEndWidget::NativeConstruct()
{
	Super::NativeConstruct();
	GameFlow = GetGameInstance()->GetSubsystem<UWxGameFlowSubsystem>();
	DisplayedPage = INDEX_NONE;
	GameFlow->OnFrontEndChanged().RemoveAll(this);
	GameFlow->OnFrontEndChanged().AddUObject(this, &ThisClass::HandleFrontEndChanged);
	GameFlow->ResetFrontEnd();
}

void UWxFrontEndWidget::NativeDestruct()
{
	if (GameFlow)
	{
		GameFlow->OnFrontEndChanged().RemoveAll(this);
		GameFlow->ResetFrontEnd();
		GameFlow = nullptr;
	}
	Super::NativeDestruct();
}

UWidget* UWxFrontEndWidget::NativeGetDesiredFocusTarget() const
{
	if (DisplayedPage == 1)
	{
		return TemplatePlayerButton;
	}
	if (DisplayedPage == 2)
	{
		return CombatButton;
	}
	return NewGame;
}

void UWxFrontEndWidget::HandleNewGame()
{
	if (GameFlow)
	{
		GameFlow->BeginCharacterSelection();
	}
}

void UWxFrontEndWidget::HandleQuitGame()
{
	if (GameFlow && !GameFlow->IsFrontEndInputBlocked())
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}

void UWxFrontEndWidget::HandleSelectCharacter(int32 Index)
{
	if (GameFlow && CharacterOptions.IsValidIndex(Index))
	{
		GameFlow->SelectCharacter(CharacterOptions[Index]);
	}
}

void UWxFrontEndWidget::HandleSelectDestination(int32 Index)
{
	if (!GameFlow || !DestinationOptions.IsValidIndex(Index))
	{
		return;
	}
	UWxUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UWxUIManagerSubsystem>();
	if (UIManager && GameFlow->SelectDestination(DestinationOptions[Index]))
	{
		UIManager->ShowConfirmation(UWxGamePopupDescriptor::CreateConfirmationOkCancel(
			LOCTEXT("StartGame", "Start Game"), GameFlow->GetStartConfirmationText()),
			FWxPopupResultDelegate::CreateUObject(this, &ThisClass::HandleStartResult));
	}
}

void UWxFrontEndWidget::HandleStartResult(EWxPopupResult Result)
{
	if (GameFlow)
	{
		GameFlow->ResolveStartConfirmation(Result == EWxPopupResult::Confirmed);
	}
}

void UWxFrontEndWidget::HandleFrontEndChanged()
{
	const bool bEnabled = !GameFlow->IsFrontEndInputBlocked();
	StatusText->SetText(GameFlow->GetStatusText());
	NewGame->SetIsEnabled(bEnabled);
	QuitGame->SetIsEnabled(bEnabled);
	TemplatePlayerButton->SetIsEnabled(bEnabled);
	BP_HGTest->SetIsEnabled(bEnabled);
	CombatButton->SetIsEnabled(bEnabled);
	OpenWorldButton->SetIsEnabled(bEnabled);

	const EWxFrontEndStep Step = GameFlow->GetFrontEndStep();
	const int32 Page = Step == EWxFrontEndStep::Main ? 0 : Step == EWxFrontEndStep::Character ? 1 : 2;
	if (DisplayedPage != Page)
	{
		DisplayedPage = Page;
		PageSwitcher->SetActiveWidgetIndex(Page);
		if (bEnabled && GetOwningPlayer())
		{
			NativeGetDesiredFocusTarget()->SetUserFocus(GetOwningPlayer());
		}
	}
}

#undef LOCTEXT_NAMESPACE
