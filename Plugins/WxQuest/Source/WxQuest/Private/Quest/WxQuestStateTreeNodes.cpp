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
	FText GetTargetText(const FUniversalObjectLocator& Locator)
	{
		if (Locator.IsEmpty())
		{
			return INVTEXT("unset");
		}

		if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
		{
			return FText::FromString(Actor->GetActorLabel());
		}

		// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
		const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
		const FActorLocatorFragment* Payload = nullptr;
		if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
		{
			const FString SubPath = Payload->Path.GetSubPathString();
			int32 DotIndex = INDEX_NONE;
			return FText::FromString(SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath);
		}

		return INVTEXT("unresolved");
	}
#endif
}

// ── SetQuestTitle ─────────────────────────────────────────────────────────────

FWxStateTreeTask_SetQuestTitle::FWxStateTreeTask_SetQuestTitle()
{
	// 진입 시 1회 등록만 하므로 틱이 불필요하다.
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	// 표시용 부수효과라 상태 완료를 판정해선 안 된다(헤더 주석 참조).
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestTitle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWxQuestComponent* QuestComponent = GetQuestComponent(Context);
	if (!QuestComponent)
	{
		// 이 태스크는 완료 판정 대상이 아니라 엔진이 반환 상태를 무시한다 — 로그가 유일한 진단 수단이다.
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
	// 완료 없이 머무는 태스크다. 재선택마다 재진입하면 ExitState 가 목표를 걷어가 표시가 깜빡인다.
	bShouldStateChangeOnReselect = false;

	// 진입·이탈에서만 저널을 건드리므로 틱이 불필요하다.
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	// 표시용 부수효과라 상태 완료를 판정해선 안 된다(헤더 주석 참조).
	bConsideredForCompletion = false;
#endif
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

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_SetQuestObjective::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 자기가 등록한 기록만 근거로 걷어간다.
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

	// 대상 미지정은 완료될 수 없는 잘못된 조립이다. 침묵 대기 대신 경고를 남긴다.
	if (Instance.Target.Locator.IsEmpty())
	{
		UE_LOG(LogWxQuest, Warning, TEXT("Wait Move To Target: 대상이 지정되지 않음(Target 빈 로케이터)."));
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

	// 미해석(빈 로케이터·스트리밍 아웃)·폰 부재 동안은 판정 없이 대기한다.
	const AActor* Target = ResolveTargetActor(Instance.Target.Locator, Owner);
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Target || !Pawn)
	{
		return EStateTreeRunStatus::Running;
	}

	if (FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) <= Instance.AcceptRadius)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitMoveToTarget::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wait Move To Target ({0})"), GetTargetText(InstanceData->Target.Locator));
}
#endif

// ── StartNextQuest ────────────────────────────────────────────────────────────

FWxStateTreeTask_StartNextQuest::FWxStateTreeTask_StartNextQuest()
{
	// 진입 시 1회 예약만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_StartNextQuest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWxQuestComponent* QuestComponent = GetQuestComponent(Context);
	if (!QuestComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 빈 지정은 컴포넌트가 무시하므로 체인 종점 처리도 같은 호출로 수렴한다.
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	QuestComponent->RequestStartQuest(Instance.NextQuest);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_StartNextQuest::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText NextQuestText = InstanceData->NextQuest.IsNull() ? INVTEXT("none") : FText::FromString(InstanceData->NextQuest.GetAssetName());
	return FText::Format(INVTEXT("Start Next Quest ({0})"), NextQuestText);
}
#endif
