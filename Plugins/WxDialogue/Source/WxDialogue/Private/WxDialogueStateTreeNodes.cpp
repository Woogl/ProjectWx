// Copyright Woogle. All Rights Reserved.

#include "WxDialogueStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxDialogueModule.h"
#include "WxDialogueSessionComponent.h"

namespace
{
	/** 로컬 플레이어(0번 컨트롤러)의 대화 세션. 세션은 대화를 건 플레이어의 컨트롤러에 있다. */
	UWxDialogueSessionComponent* FindDialogueSession(const AActor* Owner)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
		return PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	}

#if WITH_EDITOR
	/** 대화 행의 표시명. 미지정이면 unset. */
	FText GetRowText(const FDataTableRowHandle& Row)
	{
		return Row.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(Row.RowName);
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

	// 행 미지정은 어떤 대사와도 같지 않아 완료될 수 없는 잘못된 조립이다. 침묵 대기 대신 경고를 남긴다.
	if (!Instance.DialogueRow.DataTable || Instance.DialogueRow.RowName.IsNone())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Wait Dialogue Completed: 대기할 대화가 지정되지 않음(DialogueRow)."));
	}

	// 이전 실행의 잔존 기록을 비운다. 진입 이전의 대화는 세지 않는다.
	Instance.bObservedDialogue = false;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitDialogueCompleted::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 세션 부재(컨트롤러 미생성 등) 동안은 판정 없이 대기한다.
	const UWxDialogueSessionComponent* Session = FindDialogueSession(Cast<AActor>(Context.GetOwner()));
	if (!Session)
	{
		return EStateTreeRunStatus::Running;
	}

	// 지정 대사가 화면에 있으면 목격을 기록한다. 대화 중이 아닐 때의 빈 핸들이 미지정 인자와 같아 보이므로 활성 대화만 맞춰 본다.
	if (Session->HasActiveDialogue())
	{
		if (Session->GetCurrentRowHandle() == Instance.DialogueRow)
		{
			Instance.bObservedDialogue = true;
		}
	}
	// 목격한 대화가 닫히면 완주다. 지정 행 다음 대사로 넘어간 것만으로는 아직 아니다 — 대화창이 닫힌 뒤 단계가 넘어가야 연출이 끊기지 않는다.
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

	return FText::Format(INVTEXT("Wait Dialogue Completed ({0})"), GetRowText(InstanceData->DialogueRow));
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

	// 대상 없는 대사다 — 카메라는 플레이어에 머문다. 관찰자(Wait Dialogue Completed)는 대상이 아니라 행으로 판정하므로 이 대화도 게이트로 쓸 수 있다.
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

	return FText::Format(INVTEXT("Play Dialogue ({0})"), GetRowText(InstanceData->StartRow));
}
#endif
