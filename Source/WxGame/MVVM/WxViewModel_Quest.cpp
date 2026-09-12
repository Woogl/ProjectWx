// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Quest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "MVVM/WxViewModel_QuestObjective.h"
#include "Quest/WxQuestComponent.h"

void UWxViewModel_Quest::Initialize(UWxQuestComponent* InQuestComponent)
{
	Deinitialize();
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
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_SET_PROPERTY_VALUE(bHasActiveQuest, false);
		UE_MVVM_SET_PROPERTY_VALUE(QuestTitle, FText::GetEmpty());
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Objectives);
	}
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
	if (!UserWidget || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_Quest::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	const UWorld* World = UserWidget ? UserWidget->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	UWxQuestComponent* QuestComponent = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr;

	// 퀘스트 소스가 늦게 준비되면 호출 측에서 이 인스턴스에 Initialize 로 주입한다.
	UWxViewModel_Quest* ViewModel = NewObject<UWxViewModel_Quest>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
	ViewModel->Initialize(QuestComponent);
	return ViewModel;
}

void UWxViewModelResolver_Quest::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_Quest* Quest = Cast<UWxViewModel_Quest>(ViewModel))
	{
		Quest->Deinitialize();
	}
}
