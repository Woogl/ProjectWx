// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_PlayDialogue.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "WxDialogueModule.h"
#include "WxDialogueSessionComponent.h"

FWxStateTreeTask_PlayDialogue::FWxStateTreeTask_PlayDialogue()
{
	bShouldCallTick = false;

	// 진행 중인 대사를 같은 상태의 재선택으로 처음부터 다시 열지 않는다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (!Instance.StartRow.DataTable || Instance.StartRow.RowName.IsNone())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 시작 행이 지정되지 않음(StartRow)."));
		return EStateTreeRunStatus::Failed;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
	UWxDialogueSessionComponent* Session = PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 0번 컨트롤러에서 대화 세션을 찾지 못함."));
		return EStateTreeRunStatus::Failed;
	}

	Session->StartDialogueRow(Instance.StartRow, nullptr);

	// 소유 클라와 권위가 같은 머신이라 세션은 위 호출 안에서 열린다.
	if (!Session->HasActiveDialogue())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 대화를 열지 못함(행 없음·대사 빔): %s"), *Instance.StartRow.RowName.ToString());
		return EStateTreeRunStatus::Failed;
	}

	// 약한 실행 컨텍스트를 넘기는 것이 엔진이 제시하는 방식이라 여기선 람다를 쓴다.
	// 신호는 발화와 함께 비워지므로 상태를 먼저 떠난 노드의 등록도 남지 않는다(그 경우 이 컨텍스트가 무효라 무시된다).
	Session->OnDialogueEnded.AddLambda([WeakContext = Context.MakeWeakExecutionContext()]()
	{
		WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
	});

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayDialogue::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText RowText = InstanceData->StartRow.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(InstanceData->StartRow.RowName);
	return FText::Format(INVTEXT("대화 재생 ({0})"), RowText);
}
#endif
