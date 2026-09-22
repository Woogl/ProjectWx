// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_InteractionList.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "MVVM/WxViewModel_Interaction.h"

void UWxViewModel_InteractionList::Initialize(UWxInteractionScannerComponent* InScanner)
{
	if (!InScanner)
	{
		return;
	}

	CachedScanner = InScanner;
	InScanner->OnRowsChanged.AddDynamic(this, &ThisClass::HandleRowsChanged);

	// 구독 전에 끝난 발행이 있을 수 있으므로 현재 상태로 시드한다.
	HandleRowsChanged();
}

void UWxViewModel_InteractionList::Deinitialize()
{
	if (UWxInteractionScannerComponent* Scanner = CachedScanner.Get())
	{
		Scanner->OnRowsChanged.RemoveDynamic(this, &ThisClass::HandleRowsChanged);
	}
	CachedScanner.Reset();

	Super::Deinitialize();
}

void UWxViewModel_InteractionList::HandleRowsChanged()
{
	Entries.Reset();
	if (const UWxInteractionScannerComponent* Scanner = CachedScanner.Get())
	{
		const TArray<FText> Prompts = Scanner->GetPrompts();
		const int32 SelectedIndex = Scanner->GetSelectedIndex();
		for (int32 Index = 0; Index < Prompts.Num(); ++Index)
		{
			UWxViewModel_Interaction* Entry = NewObject<UWxViewModel_Interaction>(this);
			Entry->Prompt = Prompts[Index];
			Entry->bSelected = Index == SelectedIndex;
			Entries.Add(Entry);
		}
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
}

void UWxViewModel_InteractionList::RequestInteract()
{
	if (UWxInteractionScannerComponent* Scanner = CachedScanner.Get())
	{
		Scanner->TryInteractSelected();
	}
}

void UWxViewModel_InteractionList::RequestCycle(int32 Delta)
{
	if (UWxInteractionScannerComponent* Scanner = CachedScanner.Get())
	{
		Scanner->CycleSelection(Delta);
	}
}

UObject* UWxViewModelResolver_InteractionList::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	// 스캐너가 없는 PC일 수 있으므로 Outer는 PC로 잡는다.
	UWxViewModel_InteractionList* ViewModel = NewObject<UWxViewModel_InteractionList>(PC);
	ViewModel->Initialize(PC->FindComponentByClass<UWxInteractionScannerComponent>());
	return ViewModel;
}

void UWxViewModelResolver_InteractionList::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_InteractionList* InteractionList = Cast<UWxViewModel_InteractionList>(ViewModel))
	{
		InteractionList->Deinitialize();
	}
}
