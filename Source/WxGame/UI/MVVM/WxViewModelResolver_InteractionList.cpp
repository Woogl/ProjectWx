// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModelResolver_InteractionList.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "UI/MVVM/WxViewModel_InteractionList.h"

UObject* UWxViewModelResolver_InteractionList::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWxViewModel_InteractionList* ViewModel = NewObject<UWxViewModel_InteractionList>(const_cast<UUserWidget*>(UserWidget));
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (UWxInteractionScannerComponent* Scanner = PC ? PC->FindComponentByClass<UWxInteractionScannerComponent>() : nullptr)
	{
		const FWxOnInteractionRowsChanged::FDelegate ApplyRows = FWxOnInteractionRowsChanged::FDelegate::CreateWeakLambda(ViewModel, [ViewModel, Scanner]
		{
			ViewModel->SetRows(Scanner->GetPrompts(), Scanner->GetSelectedIndex());
		});

		// 위젯보다 먼저 모인 행도 보여 준다.
		ApplyRows.Execute();
		Scanner->OnRowsChanged.Add(ApplyRows);
		ViewModel->SetScanner(Scanner);
	}
	return ViewModel;
}

void UWxViewModelResolver_InteractionList::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	const UUserWidget* UserWidget = ViewModel ? ViewModel->GetTypedOuter<UUserWidget>() : nullptr;
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (UWxInteractionScannerComponent* Scanner = PC ? PC->FindComponentByClass<UWxInteractionScannerComponent>() : nullptr)
	{
		Scanner->OnRowsChanged.RemoveAll(ViewModel);
	}
}
