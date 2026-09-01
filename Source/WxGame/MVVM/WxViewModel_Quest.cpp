// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Quest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "MVVM/WxViewModel_QuestObjective.h"
#include "Quest/WxQuestComponent.h"

void UWxViewModel_Quest::Initialize(UWxQuestComponent* InQuestComponent)
{
	if (!InQuestComponent)
	{
		return;
	}

	CachedQuestComponent = InQuestComponent;

	InQuestComponent->OnJournalChanged.AddDynamic(this, &ThisClass::HandleJournalChanged);

	// 구독 전에 끝난 broadcast 가 있을 수 있으므로 현재 저널로 시드한다.
	HandleJournalChanged();
}

void UWxViewModel_Quest::Deinitialize()
{
	if (UWxQuestComponent* QuestComponent = CachedQuestComponent.Get())
	{
		QuestComponent->OnJournalChanged.RemoveDynamic(this, &ThisClass::HandleJournalChanged);
	}
	CachedQuestComponent.Reset();

	Objectives.Reset();

	Super::Deinitialize();
}

void UWxViewModel_Quest::HandleJournalChanged()
{
	const UWxQuestComponent* QuestComponent = CachedQuestComponent.Get();
	if (!QuestComponent)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(bHasActiveQuest, QuestComponent->HasActiveQuest());
	UE_MVVM_SET_PROPERTY_VALUE(QuestTitle, QuestComponent->GetQuestTitle());
	RebuildObjectives(QuestComponent->GetObjectiveTexts());
}

void UWxViewModel_Quest::RebuildObjectives(const TArray<FText>& InObjectiveTexts)
{
	Objectives.Reset(InObjectiveTexts.Num());
	for (const FText& ObjectiveText : InObjectiveTexts)
	{
		UWxViewModel_QuestObjective* Objective = NewObject<UWxViewModel_QuestObjective>(this);
		Objective->SetObjectiveText(ObjectiveText);
		Objectives.Add(Objective);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Objectives);
}

UObject* UWxViewModelResolver_Quest::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const UWorld* World = UserWidget ? UserWidget->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	UWxQuestComponent* QuestComponent = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr;
	if (!QuestComponent)
	{
		return nullptr;
	}

	// 컴포넌트는 GameState 소유라 월드 수명과 같으며, 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_Quest* ViewModel = NewObject<UWxViewModel_Quest>(QuestComponent);
	ViewModel->Initialize(QuestComponent);
	return ViewModel;
}
