// Copyright Woogle. All Rights Reserved.

#include "Quest/WxQuestComponent.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StateTreeComponent.h"
#include "Engine/World.h"
#include "Quest/WxQuestStateTree.h"
#include "TimerManager.h"
#include "WxQuestModule.h"

UWxQuestComponent::UWxQuestComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 틱은 러너가 스스로 돌린다. 본 컴포넌트는 위임·저널만 담당한다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxQuestComponent::StartQuest(UWxQuestStateTree* QuestAsset)
{
	// 러너는 권위에서만 생성되므로 비-권위 머신의 호출은 자연히 노옵이다.
	if (!QuestStateTree || !QuestAsset)
	{
		return;
	}

	// 정지 → 교체 → 시작. 엔진이 Running 중 SetStateTree 를 거부하므로 반드시 이 순서다.
	// 정지가 저널 정리(HandleStateTreeRunStatusChanged)를 발화시키고, 새 퀘스트 진행이 저널을 다시 채운다.
	QuestStateTree->StopLogic(TEXT("StartQuest"));
	QuestStateTree->SetStateTree(QuestAsset);
	QuestStateTree->StartLogic();
}

void UWxQuestComponent::RequestStartQuest(TSoftObjectPtr<UWxQuestStateTree> QuestAsset)
{
	if (QuestAsset.IsNull())
	{
		return;
	}

	// 타이머 대기 중 GC 로 로드가 풀릴 수 있어 포인터가 아닌 소프트 참조를 넘기고 실행 시점에 로드한다.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UWxQuestComponent::HandleDeferredStartQuest, QuestAsset));
}

void UWxQuestComponent::SendQuestEvent(FGameplayTag EventTag)
{
	if (QuestStateTree)
	{
		QuestStateTree->SendStateTreeEvent(EventTag);
	}
}

void UWxQuestComponent::SetJournal(const FText& InQuestTitle)
{
	QuestTitle = InQuestTitle;
	ObjectiveText = FText::GetEmpty();
	bHasActiveQuest = true;
	OnJournalChanged.Broadcast();
}

void UWxQuestComponent::SetObjective(const FText& InObjectiveText)
{
	ObjectiveText = InObjectiveText;
	OnJournalChanged.Broadcast();
}

void UWxQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	// 퀘스트 진행은 월드 부수효과(스폰·보상)를 동반하는 서버 권위 사건이라 러너를 권위에서만 생성한다.
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 자동 시작을 끈 순정 러너를 런타임 부착한다. 실행할 에셋은 StartQuest 가 그때그때 지정한다.
	QuestStateTree = NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"));
	QuestStateTree->SetStartLogicAutomatically(false);
	QuestStateTree->RegisterComponent();
	QuestStateTree->OnStateTreeRunStatusChanged.AddDynamic(this, &UWxQuestComponent::HandleStateTreeRunStatusChanged);

	// 첫 탑재 지정은 에셋 자신에게 있다(bAutoStart) — 레지스트리 발견이라 컴포넌트는 여전히 특정 에셋을 모른다.
	FARFilter Filter;
	Filter.ClassPaths.Add(UWxQuestStateTree::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	TArray<FAssetData> QuestAssets;
	AssetRegistry.GetAssets(Filter, QuestAssets);

	// 발견 0건 자체는 정상(퀘스트 없는 상태)이나, 스캔 미완이 원인이면 침묵이 오진을 만들므로 구분해 남긴다.
	if (QuestAssets.IsEmpty() && AssetRegistry.IsLoadingAssets())
	{
		UE_LOG(LogWxQuest, Warning, TEXT("퀘스트 에셋 발견 0건 — 애셋 레지스트리 스캔 미완이라 자동 탑재가 누락됐을 수 있음."));
	}

	// 활성 1개 원칙이라 자동 탑재도 1개만 성립한다.
	// 복수 선언은 잘못된 조립이므로 경고하되, 경로 사전순 첫 에셋을 골라 결정성은 유지한다(레지스트리 결과는 순서 비보장).
	const FAssetData* AutoStartQuest = nullptr;
	int32 AutoStartCount = 0;
	for (const FAssetData& QuestAsset : QuestAssets)
	{
		// 태그 부재(프로퍼티 도입 전 저장분)는 false 와 같다.
		bool bAutoStart = false;
		QuestAsset.GetTagValue(GET_MEMBER_NAME_CHECKED(UWxQuestStateTree, bAutoStart), bAutoStart);
		if (!bAutoStart)
		{
			continue;
		}

		++AutoStartCount;
		if (!AutoStartQuest || QuestAsset.GetSoftObjectPath().LexicalLess(AutoStartQuest->GetSoftObjectPath()))
		{
			AutoStartQuest = &QuestAsset;
		}
	}

	if (!AutoStartQuest)
	{
		return;
	}

	if (AutoStartCount > 1)
	{
		UE_LOG(LogWxQuest, Warning, TEXT("bAutoStart 퀘스트가 %d개 — 활성 1개 원칙에 따라 %s 만 탑재함."), AutoStartCount, *AutoStartQuest->GetSoftObjectPath().ToString());
	}

	// BeginPlay 중의 초기화 순서를 타지 않도록 기존 지연 시작 경로(다음 틱 로드·시작)로 탑재한다.
	RequestStartQuest(TSoftObjectPtr<UWxQuestStateTree>(AutoStartQuest->GetSoftObjectPath()));
}

void UWxQuestComponent::HandleStateTreeRunStatusChanged(EStateTreeRunStatus StateTreeRunStatus)
{
	// 저널 수명을 퀘스트 실행과 일치시킨다 — 완료(Succeeded)·실패(Failed)·교체(Stopped) 세 종료 경로가 전부 여기로 수렴한다.
	if (StateTreeRunStatus != EStateTreeRunStatus::Running)
	{
		ClearJournal();
	}
}

void UWxQuestComponent::HandleDeferredStartQuest(TSoftObjectPtr<UWxQuestStateTree> QuestAsset)
{
	StartQuest(QuestAsset.LoadSynchronous());
}

void UWxQuestComponent::ClearJournal()
{
	if (!bHasActiveQuest)
	{
		return;
	}

	bHasActiveQuest = false;
	QuestTitle = FText::GetEmpty();
	ObjectiveText = FText::GetEmpty();
	OnJournalChanged.Broadcast();
}
