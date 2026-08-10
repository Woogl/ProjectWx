// Copyright Woogle. All Rights Reserved.

#include "Quest/WxQuestStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Quest/WxQuestComponent.h"
#include "Quest/WxQuestStateTree.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxQuestModule.h"

namespace
{
	/** 컨텍스트 오너(GameState)에서 퀘스트 컴포넌트를 찾는다. 퀘스트 러너 밖 조립이면 null. */
	UWxQuestComponent* GetQuestComponent(const FStateTreeExecutionContext& Context)
	{
		const AActor* Owner = Cast<AActor>(Context.GetOwner());
		return Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
	}

	/** 로케이터를 ST 오너 컨텍스트로 대상 액터로 해석한다. 빈 로케이터·미로드(스트리밍 아웃)는 nullptr. */
	AActor* ResolveTargetActor(const FUniversalObjectLocator& Locator, AActor* Owner)
	{
		return Cast<AActor>(Locator.SyncFind(Owner));
	}

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FString GetTargetDisplayName(const FUniversalObjectLocator& Locator)
	{
		if (Locator.IsEmpty())
		{
			return TEXT("unset");
		}

		if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
		{
			return Actor->GetActorLabel();
		}

		// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
		const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
		const FActorLocatorFragment* Payload = nullptr;
		if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
		{
			const FString SubPath = Payload->Path.GetSubPathString();
			int32 DotIndex = INDEX_NONE;
			return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
		}

		return TEXT("unresolved");
	}

	/** 지정 목록의 표시 텍스트. 라벨 3개까지 나열하고 초과분은 +N 로 줄인다. 빈 배열은 none. */
	FText GetTargetsText(const TArray<FUniversalObjectLocator>& Targets)
	{
		if (Targets.IsEmpty())
		{
			return INVTEXT("none");
		}

		constexpr int32 MaxNames = 3;
		TArray<FString> Names;
		for (int32 Index = 0; Index < Targets.Num() && Index < MaxNames; ++Index)
		{
			Names.Add(GetTargetDisplayName(Targets[Index]));
		}

		FString Joined = FString::Join(Names, TEXT(", "));
		if (Targets.Num() > MaxNames)
		{
			Joined += FString::Printf(TEXT(" +%d"), Targets.Num() - MaxNames);
		}
		return FText::FromString(Joined);
	}
#endif
}

// ── SetQuestTitle ─────────────────────────────────────────────────────────────

FWxStateTreeTask_SetQuestTitle::FWxStateTreeTask_SetQuestTitle()
{
	// 진입 시 1회 등록만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestTitle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWxQuestComponent* QuestComponent = GetQuestComponent(Context);
	if (!QuestComponent)
	{
		UE_LOG(LogWxQuest, Warning, TEXT("Set Quest Title: 오너 %s 에서 퀘스트 컴포넌트를 찾지 못함(퀘스트 러너 밖 조립). 제목이 등록되지 않는다."),
			*GetNameSafe(Context.GetOwner()));
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	QuestComponent->SetQuestTitle(Instance.QuestTitle);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SetQuestTitle::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText QuestTitle = InstanceData->QuestTitle.IsEmpty() ? INVTEXT("none") : InstanceData->QuestTitle;
	return FText::Format(INVTEXT("Set Quest Title (\"{0}\")"), QuestTitle);
}
#endif

// ── SetQuestObjective ─────────────────────────────────────────────────────────

FWxStateTreeTask_SetQuestObjective::FWxStateTreeTask_SetQuestObjective()
{
	// 진입·이탈에서만 저널을 건드리므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestObjective::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWxQuestComponent* QuestComponent = GetQuestComponent(Context);
	if (!QuestComponent)
	{
		// 이 태스크는 완료 판정 대상이 아니라 엔진이 반환 상태를 무시한다 — 로그가 유일한 진단 수단이다.
		UE_LOG(LogWxQuest, Warning, TEXT("Set Quest Objective: 오너 %s 에서 퀘스트 컴포넌트를 찾지 못함(퀘스트 러너 밖 조립). 목표가 등록되지 않는다."),
			*GetNameSafe(Context.GetOwner()));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.ObjectiveHandle = QuestComponent->AddObjective(Instance.ObjectiveText);

	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_SetQuestObjective::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (UWxQuestComponent* QuestComponent = GetQuestComponent(Context))
	{
		QuestComponent->RemoveObjective(Instance.ObjectiveHandle);
	}
	Instance.ObjectiveHandle = INDEX_NONE;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SetQuestObjective::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText ObjectiveText = InstanceData->ObjectiveText.IsEmpty() ? INVTEXT("none") : InstanceData->ObjectiveText;
	return FText::Format(INVTEXT("Set Quest Objective (\"{0}\")"), ObjectiveText);
}
#endif

// ── WaitMoveToTarget ──────────────────────────────────────────────────────────

FWxStateTreeTask_WaitMoveToTarget::FWxStateTreeTask_WaitMoveToTarget()
{
	// 도달 대기 중 같은 상태가 재선택되어도 진행을 끊을 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitMoveToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.Targets.IsEmpty())
	{
		UE_LOG(LogWxQuest, Warning, TEXT("Wait Move To Target: 도달을 판정할 대상이 지정되지 않음."));
	}
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		if (Locator.IsEmpty())
		{
			UE_LOG(LogWxQuest, Warning, TEXT("Wait Move To Target: 빈 로케이터 항목이 있음(지정 %d개)."), Instance.Targets.Num());
			break;
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitMoveToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Running;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Running;
	}

	// 미해석(빈 로케이터·스트리밍 아웃)인 대상은 그 자리만 판정에서 빠진다.
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		const AActor* Target = ResolveTargetActor(Locator, Owner);
		if (Target && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) <= Instance.AcceptRadius)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitMoveToTarget::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wait Move To Target ({0})"), GetTargetsText(InstanceData->Targets));
}
#endif

// ── ActivateNextQuest ────────────────────────────────────────────────────────────

FWxStateTreeTask_ActivateNextQuest::FWxStateTreeTask_ActivateNextQuest()
{
	// 진입 시 1회 예약만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_ActivateNextQuest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWxQuestComponent* QuestComponent = GetQuestComponent(Context);
	if (!QuestComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 빈 지정은 컴포넌트가 무시하므로 체인 종점 처리도 같은 호출로 수렴한다.
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	QuestComponent->RequestActivateQuest(Instance.NextQuest);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ActivateNextQuest::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText NextQuestText = InstanceData->NextQuest.IsNull() ? INVTEXT("none") : FText::FromString(InstanceData->NextQuest.GetAssetName());
	return FText::Format(INVTEXT("Activate Next Quest ({0})"), NextQuestText);
}
#endif
