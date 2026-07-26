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

// ── SetQuestObjective ─────────────────────────────────────────────────────────

FWxStateTreeTask_SetQuestObjective::FWxStateTreeTask_SetQuestObjective()
{
	// 진입 시 1회 갱신만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetQuestObjective::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWxQuestComponent* QuestComponent = GetQuestComponent(Context);
	if (!QuestComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 제목이 있으면 저널 신규 등록(목표 비움 포함) 후 목표가 있으면 이어서 채운다. 제목이 비면 목표만 갱신한다(빈 문구=목표 비우기).
	if (!Instance.QuestTitle.IsEmpty())
	{
		QuestComponent->SetJournal(Instance.QuestTitle);
		if (!Instance.ObjectiveText.IsEmpty())
		{
			QuestComponent->SetObjective(Instance.ObjectiveText);
		}
	}
	else
	{
		QuestComponent->SetObjective(Instance.ObjectiveText);
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SetQuestObjective::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText ObjectiveText = InstanceData->ObjectiveText.IsEmpty() ? INVTEXT("none") : InstanceData->ObjectiveText;
	if (InstanceData->QuestTitle.IsEmpty())
	{
		return FText::Format(INVTEXT("Set Quest Objective ({0})"), ObjectiveText);
	}
	return FText::Format(INVTEXT("Set Quest Objective ({0}: {1})"), InstanceData->QuestTitle, ObjectiveText);
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
