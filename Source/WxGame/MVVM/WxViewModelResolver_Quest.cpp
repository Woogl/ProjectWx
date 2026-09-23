// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Quest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "MVVM/WxViewModel_Quest.h"
#include "Quest/WxQuestComponent.h"

UObject* UWxViewModelResolver_Quest::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWxViewModel_Quest* ViewModel = NewObject<UWxViewModel_Quest>(const_cast<UUserWidget*>(UserWidget));
	const UWorld* World = UserWidget ? UserWidget->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (UWxQuestComponent* Quest = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr)
	{
		const FWxOnQuestJournalChanged::FDelegate ApplyJournal = FWxOnQuestJournalChanged::FDelegate::CreateWeakLambda(ViewModel, [ViewModel, Quest]
		{
			ViewModel->SetJournal(Quest->HasActiveQuest(), Quest->GetQuestTitle(), Quest->GetObjectiveTexts());
		});

		// 위젯보다 먼저 등록된 저널도 보여 준다.
		ApplyJournal.Execute();
		Quest->OnJournalChanged.Add(ApplyJournal);
	}
	return ViewModel;
}

void UWxViewModelResolver_Quest::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	const UWorld* World = ViewModel ? ViewModel->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (UWxQuestComponent* Quest = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr)
	{
		Quest->OnJournalChanged.RemoveAll(ViewModel);
	}
}
