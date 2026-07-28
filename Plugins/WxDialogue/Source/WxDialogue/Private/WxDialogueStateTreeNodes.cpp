// Copyright Woogle. All Rights Reserved.

#include "WxDialogueStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxDialogueModule.h"
#include "WxDialogueSessionComponent.h"

namespace
{
	/** 로케이터를 ST 오너 컨텍스트로 대상 액터로 해석한다. 빈 로케이터·미로드(스트리밍 아웃)는 nullptr. */
	AActor* ResolveTargetActor(const FUniversalObjectLocator& Locator, AActor* Owner)
	{
		return Cast<AActor>(Locator.SyncFind(Owner));
	}

	/** 로컬 플레이어(0번 컨트롤러)의 대화 세션. 세션은 대화를 건 플레이어의 컨트롤러에 있다. */
	UWxDialogueSessionComponent* FindDialogueSession(const AActor* Owner)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
		return PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
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

// ── WaitDialogueCompleted ─────────────────────────────────────────────────────

FWxStateTreeTask_WaitDialogueCompleted::FWxStateTreeTask_WaitDialogueCompleted()
{
	// 완주 대기 중 같은 상태가 재선택되어도 목격 기록을 끊을 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitDialogueCompleted::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 대상 미지정은 완료될 수 없는 잘못된 조립이다. 침묵 대기 대신 경고를 남긴다.
	if (Instance.Target.Locator.IsEmpty())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Wait Dialogue Completed: 대상이 지정되지 않음(Target 빈 로케이터)."));
	}

	// 이전 실행의 잔존 기록을 비운다. 진입 이전의 대화는 세지 않는다.
	Instance.bObservedDialogue = false;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitDialogueCompleted::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Running;
	}

	// 미해석(빈 로케이터·스트리밍 아웃)·세션 부재 동안은 판정 없이 대기한다.
	const AActor* Target = ResolveTargetActor(Instance.Target.Locator, Owner);
	const UWxDialogueSessionComponent* Session = FindDialogueSession(Owner);
	if (!Target || !Session)
	{
		return EStateTreeRunStatus::Running;
	}

	// 대상과 대화 중이면 목격을 기록하고, 목격 후 대상과의 대화가 아니게 되면(종료·전환) 완주다.
	if (Session->GetCurrentDialogueTarget() == Target)
	{
		Instance.bObservedDialogue = true;
	}
	else if (Instance.bObservedDialogue)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitDialogueCompleted::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wait Dialogue Completed ({0})"), GetTargetText(InstanceData->Target.Locator));
}
#endif

// ── PlayDialogue ──────────────────────────────────────────────────────────────

FWxStateTreeTask_PlayDialogue::FWxStateTreeTask_PlayDialogue()
{
	// 진행 중인 대사를 같은 상태의 재선택으로 처음부터 다시 열지 않는다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 아래 셋은 모두 완주할 수 없는 잘못된 조립이다 — 대사 없이 상태에 눌러앉는 대신 실패를 낸다.
	if (!Instance.StartRow.DataTable || Instance.StartRow.RowName.IsNone())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 시작 행이 지정되지 않음(StartRow)."));
		return EStateTreeRunStatus::Failed;
	}

	UWxDialogueSessionComponent* Session = FindDialogueSession(Cast<AActor>(Context.GetOwner()));
	if (!Session)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 0번 컨트롤러에서 대화 세션을 찾지 못함."));
		return EStateTreeRunStatus::Failed;
	}

	// 대상 없는 대사다 — 카메라는 플레이어에 머물고 관찰자(Wait Dialogue Completed)에도 잡히지 않는다.
	Session->StartDialogueRow(Instance.StartRow, nullptr);

	// 소유 클라와 권위가 같은 머신이라 세션은 위 호출 안에서 열린다. 열리지 않았다면 행이 없거나 대사가 빈 것이다.
	if (!Session->HasActiveDialogue())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 대화를 열지 못함(행 없음·대사 빔): %s"), *Instance.StartRow.RowName.ToString());
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 세션이 사라졌다면(컨트롤러 교체 등) 더 기다릴 대화가 없으므로 종료와 같이 본다.
	const UWxDialogueSessionComponent* Session = FindDialogueSession(Cast<AActor>(Context.GetOwner()));

	return Session && Session->HasActiveDialogue() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayDialogue::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FName RowName = InstanceData->StartRow.RowName;

	return FText::Format(INVTEXT("Play Dialogue ({0})"), RowName.IsNone() ? INVTEXT("unset") : FText::FromName(RowName));
}
#endif
