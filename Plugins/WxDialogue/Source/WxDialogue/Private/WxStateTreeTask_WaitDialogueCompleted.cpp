// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_WaitDialogueCompleted.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxDialogueModule.h"
#include "WxDialogueSessionComponent.h"

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
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
	const UWxDialogueSessionComponent* Session = PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		return EStateTreeRunStatus::Running;
	}

	// 대화 중이 아닐 때의 빈 핸들이 미지정 인자와 같아 보이므로 활성 대화만 맞춰 본다.
	if (Session->HasActiveDialogue())
	{
		if (Session->GetCurrentRowHandle() == Instance.DialogueRow)
		{
			Instance.bObservedDialogue = true;
		}
	}
	// 지정 행 다음 대사로 넘어간 것만으로는 아직 완주가 아니다 — 대화창이 닫힌 뒤 단계가 넘어가야 연출이 끊기지 않는다.
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

	const FText RowText = InstanceData->DialogueRow.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(InstanceData->DialogueRow.RowName);
	return FText::Format(INVTEXT("Wait Dialogue Completed ({0})"), RowText);
}
#endif
