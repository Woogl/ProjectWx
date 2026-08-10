// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_InteractionList.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "MVVM/WxViewModel_Interaction.h"

void UWxViewModel_InteractionList::StartObserving(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	ObservedController = PC;

	// 이미 붙어 있으면(호스트에선 위젯보다 항상 먼저다) 기다릴 것 없이 바로 연결한다.
	if (UWxInteractionScannerComponent* Scanner = PC->FindComponentByClass<UWxInteractionScannerComponent>())
	{
		Initialize(Scanner);
		return;
	}

	ScannerReadyHandle = UWxInteractionScannerComponent::OnAnyScannerReady.AddUObject(this, &UWxViewModel_InteractionList::HandleScannerReady);
}

void UWxViewModel_InteractionList::Initialize(UWxInteractionScannerComponent* InScanner)
{
	if (!InScanner)
	{
		return;
	}

	Deinitialize();

	CachedScanner = InScanner;

	InScanner->OnListChanged.AddDynamic(this, &ThisClass::HandleListChanged);
	InScanner->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleSelectionChanged);

	// 구독 전에 끝난 broadcast 가 있을 수 있으므로 현재 목록/선택으로 시드한다(목록이 비면 INDEX_NONE).
	RebuildEntries(InScanner->GetPrompts());
	ApplySelection(InScanner->GetSelectedIndex());
}

void UWxViewModel_InteractionList::Deinitialize()
{
	if (UWxInteractionScannerComponent* Scanner = CachedScanner.Get())
	{
		Scanner->OnListChanged.RemoveDynamic(this, &ThisClass::HandleListChanged);
		Scanner->OnSelectionChanged.RemoveDynamic(this, &ThisClass::HandleSelectionChanged);
	}
	CachedScanner.Reset();

	if (!Entries.IsEmpty())
	{
		Entries.Reset();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
	}

	Super::Deinitialize();
}

void UWxViewModel_InteractionList::BeginDestroy()
{
	StopObserving();

	Super::BeginDestroy();
}

void UWxViewModel_InteractionList::HandleListChanged(const TArray<FText>& InPrompts)
{
	RebuildEntries(InPrompts);

	// 목록이 새 항목으로 교체되었으므로 현재 선택을 다시 적용해 bSelected 를 반영한다(정확한 값은 곧 HandleSelectionChanged 가 덮는다).
	ApplySelection(SelectedIndex);
}

void UWxViewModel_InteractionList::HandleSelectionChanged(int32 InSelectedIndex)
{
	ApplySelection(InSelectedIndex);
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

void UWxViewModel_InteractionList::HandleScannerReady(UWxInteractionScannerComponent* Scanner)
{
	// 신호는 클래스 차원이라 남의 스캐너도 온다(PIE 다중 인스턴스 포함). 관찰 중인 컨트롤러의 것만 받는다.
	if (!Scanner || Scanner->GetOwner() != ObservedController.Get())
	{
		return;
	}

	StopObserving();
	Initialize(Scanner);
}

void UWxViewModel_InteractionList::StopObserving()
{
	if (ScannerReadyHandle.IsValid())
	{
		UWxInteractionScannerComponent::OnAnyScannerReady.Remove(ScannerReadyHandle);
		ScannerReadyHandle.Reset();
	}
}

void UWxViewModel_InteractionList::RebuildEntries(const TArray<FText>& InPrompts)
{
	Entries.Reset(InPrompts.Num());
	for (const FText& Prompt : InPrompts)
	{
		UWxViewModel_Interaction* Entry = NewObject<UWxViewModel_Interaction>(this);
		Entry->SetPrompt(Prompt);
		Entries.Add(Entry);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
}

void UWxViewModel_InteractionList::ApplySelection(int32 InSelectedIndex)
{
	const int32 ClampedIndex = Entries.IsEmpty() ? INDEX_NONE : FMath::Clamp(InSelectedIndex, 0, Entries.Num() - 1);

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (UWxViewModel_Interaction* Entry = Entries[Index])
		{
			Entry->SetSelected(Index == ClampedIndex);
		}
	}

	if (SelectedIndex != ClampedIndex)
	{
		SelectedIndex = ClampedIndex;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedIndex);
	}
}

UObject* UWxViewModelResolver_InteractionList::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	// 스캐너가 아직 없을 수 있으므로 Outer 는 PC 로 잡는다.
	UWxViewModel_InteractionList* ViewModel = NewObject<UWxViewModel_InteractionList>(PC);
	ViewModel->StartObserving(PC);
	return ViewModel;
}
