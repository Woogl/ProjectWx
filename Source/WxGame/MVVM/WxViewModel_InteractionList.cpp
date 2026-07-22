// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_InteractionList.h"
#include "Blueprint/UserWidget.h"
#include "Controller/WxPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionRegistryComponent.h"
#include "MVVM/WxViewModel_Interaction.h"

void UWxViewModel_InteractionList::Initialize(UWxInteractionRegistryComponent* InRegistry)
{
	if (!InRegistry)
	{
		return;
	}

	CachedRegistry = InRegistry;

	InRegistry->OnListChanged.AddDynamic(this, &ThisClass::HandleListChanged);
	InRegistry->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleSelectionChanged);

	// 구독 전에 끝난 broadcast 가 있을 수 있으므로 현재 목록/선택으로 시드한다(목록이 비면 INDEX_NONE).
	RebuildEntries(InRegistry->GetPrompts());
	ApplySelection(InRegistry->GetSelectedIndex());
}

void UWxViewModel_InteractionList::Deinitialize()
{
	if (UWxInteractionRegistryComponent* Registry = CachedRegistry.Get())
	{
		Registry->OnListChanged.RemoveDynamic(this, &ThisClass::HandleListChanged);
		Registry->OnSelectionChanged.RemoveDynamic(this, &ThisClass::HandleSelectionChanged);
	}
	CachedRegistry.Reset();

	if (!Entries.IsEmpty())
	{
		Entries.Reset();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
	}

	Super::Deinitialize();
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
	if (UWxInteractionRegistryComponent* Registry = CachedRegistry.Get())
	{
		Registry->TryInteractSelected();
	}
}

void UWxViewModel_InteractionList::RequestCycle(int32 Delta)
{
	if (UWxInteractionRegistryComponent* Registry = CachedRegistry.Get())
	{
		Registry->CycleSelection(Delta);
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
	const AWxPlayerController* PC = UserWidget ? Cast<AWxPlayerController>(UserWidget->GetOwningPlayer()) : nullptr;
	UWxInteractionRegistryComponent* Registry = PC ? PC->GetInteractionRegistry() : nullptr;
	if (!Registry)
	{
		return nullptr;
	}

	// 위젯이 아닌 데이터 소스(레지스트리 컴포넌트)를 Outer 로 생성한다.
	// 레지스트리는 PC 소유라 폰 리스폰에도 생존하며, 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_InteractionList* ViewModel = NewObject<UWxViewModel_InteractionList>(Registry);
	ViewModel->Initialize(Registry);
	return ViewModel;
}
