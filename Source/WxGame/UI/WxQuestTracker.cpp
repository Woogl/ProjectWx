// Copyright Woogle. All Rights Reserved.

#include "UI/WxQuestTracker.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "MVVM/WxViewModel_Quest.h"
#include "Quest/WxQuestComponent.h"
#include "View/MVVMView.h"

void UWxQuestTracker::NativeConstruct()
{
	Super::NativeConstruct();
	UnbindJournal();
	const UMVVMView* View = GetExtension<UMVVMView>();
	if (!View || !View->AreSourcesInitialized())
	{
		return;
	}
	UWxViewModel_Quest* ViewModel = Cast<UWxViewModel_Quest>(View->GetViewModel(TEXT("WxViewModel_Quest")).GetObject());
	if (!ensureMsgf(ViewModel, TEXT("QuestTracker requires a WxViewModel_Quest source with Create Instance.")))
	{
		return;
	}
	QuestViewModel = ViewModel;
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	UWxQuestComponent* Quest = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr;
	ObservedQuest = Quest;
	if (Quest)
	{
		Quest->OnJournalChanged.AddUniqueDynamic(this, &ThisClass::HandleJournalChanged);
	}
	// 위젯 생성 전에 발행된 변경도 현재 저널에서 보충한다.
	HandleJournalChanged();
}

void UWxQuestTracker::NativeDestruct()
{
	UnbindJournal();
	Super::NativeDestruct();
}

void UWxQuestTracker::UnbindJournal()
{
	if (UWxQuestComponent* Quest = ObservedQuest.Get())
	{
		Quest->OnJournalChanged.RemoveDynamic(this, &ThisClass::HandleJournalChanged);
	}
	ObservedQuest.Reset();
	QuestViewModel.Reset();
}

void UWxQuestTracker::HandleJournalChanged()
{
	if (UWxViewModel_Quest* ViewModel = QuestViewModel.Get())
	{
		const UWxQuestComponent* Quest = ObservedQuest.Get();
		if (Quest)
		{
			ViewModel->SetJournal(Quest->HasActiveQuest(), Quest->GetQuestTitle(), Quest->GetObjectiveTexts());
		}
		else
		{
			ViewModel->SetJournal(false, FText::GetEmpty(), {});
		}
	}
}
