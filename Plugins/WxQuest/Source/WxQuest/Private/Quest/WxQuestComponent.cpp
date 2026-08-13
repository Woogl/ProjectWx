// Copyright Woogle. All Rights Reserved.

#include "Quest/WxQuestComponent.h"

#include "Components/StateTreeComponent.h"
#include "Engine/World.h"
#include "Quest/WxQuestStateTree.h"
#include "StateTreeReference.h"
#include "TimerManager.h"

UWxQuestComponent::UWxQuestComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 틱은 러너가 스스로 돌린다. 본 컴포넌트는 위임·저널만 담당한다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxQuestComponent::ActivateQuest(UWxQuestStateTree* QuestAsset)
{
	if (!QuestAsset)
	{
		return;
	}

	// 러너는 권위에서만 생성되므로 비-권위 머신의 호출은 자연히 노옵이다.
	if (!QuestStateTree)
	{
		return;
	}

	// 엔진이 Running 중 에셋 교체를 거부하므로 정지 → 교체 → 시작 순서를 지킨다.
	// 정지가 저널 정리(HandleStateTreeRunStatusChanged)를 발화시키고, 새 퀘스트 진행이 저널을 다시 채운다.
	FStateTreeReference Quest;
	Quest.SetStateTree(QuestAsset);
	QuestStateTree->StopLogic(TEXT("ActivateQuest"));
	QuestStateTree->SetStateTreeReference(Quest);
	QuestStateTree->StartLogic();
}

void UWxQuestComponent::RequestActivateQuest(TSoftObjectPtr<UWxQuestStateTree> QuestAsset)
{
	if (QuestAsset.IsNull())
	{
		return;
	}

	// 타이머 대기 중 GC 로 로드가 풀릴 수 있어 포인터가 아닌 소프트 참조를 넘기고 실행 시점에 로드한다.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UWxQuestComponent::HandleDeferredActivateQuest, QuestAsset));
}

void UWxQuestComponent::SetQuestTitle(const FText& InQuestTitle)
{
	QuestTitle = InQuestTitle;
	Objectives.Reset();
	bHasActiveQuest = true;
	OnJournalChanged.Broadcast();
}

int32 UWxQuestComponent::AddObjective(const FText& InObjectiveText)
{
	FWxQuestObjective& Objective = Objectives.AddDefaulted_GetRef();
	Objective.Handle = NextObjectiveHandle++;
	Objective.Text = InObjectiveText;

	OnJournalChanged.Broadcast();

	return Objective.Handle;
}

void UWxQuestComponent::RemoveObjective(int32 ObjectiveHandle)
{
	// 제목 교체·저널 정리가 목록을 통째로 비운 뒤라면 이미 사라진 핸들이 들어온다.
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (Objectives[Index].Handle == ObjectiveHandle)
		{
			Objectives.RemoveAt(Index);
			OnJournalChanged.Broadcast();
			return;
		}
	}
}

bool UWxQuestComponent::HasActiveQuest() const
{
	return bHasActiveQuest;
}

FText UWxQuestComponent::GetQuestTitle() const
{
	return QuestTitle;
}

TArray<FText> UWxQuestComponent::GetObjectiveTexts() const
{
	TArray<FText> ObjectiveTexts;
	ObjectiveTexts.Reserve(Objectives.Num());
	for (const FWxQuestObjective& Objective : Objectives)
	{
		ObjectiveTexts.Add(Objective.Text);
	}

	return ObjectiveTexts;
}

void UWxQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 실행할 에셋은 ActivateQuest 가 그때그때 지정하므로 자동 시작을 끈다.
	QuestStateTree = NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"));
	QuestStateTree->SetStartLogicAutomatically(false);
	QuestStateTree->RegisterComponent();
	QuestStateTree->OnStateTreeRunStatusChanged.AddDynamic(this, &UWxQuestComponent::HandleStateTreeRunStatusChanged);
}

void UWxQuestComponent::HandleStateTreeRunStatusChanged(EStateTreeRunStatus StateTreeRunStatus)
{
	if (StateTreeRunStatus != EStateTreeRunStatus::Running)
	{
		ClearJournal();
	}
}

void UWxQuestComponent::HandleDeferredActivateQuest(TSoftObjectPtr<UWxQuestStateTree> QuestAsset)
{
	ActivateQuest(QuestAsset.LoadSynchronous());
}

void UWxQuestComponent::ClearJournal()
{
	if (!bHasActiveQuest)
	{
		return;
	}

	bHasActiveQuest = false;
	QuestTitle = FText::GetEmpty();
	Objectives.Reset();
	OnJournalChanged.Broadcast();
}
