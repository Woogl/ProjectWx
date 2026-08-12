// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_PlayDialogue.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxDialogueModule.h"
#include "WxDialogueSessionComponent.h"

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

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
	UWxDialogueSessionComponent* Session = PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
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
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
	const UWxDialogueSessionComponent* Session = PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;

	return Session && Session->HasActiveDialogue() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayDialogue::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText RowText = InstanceData->StartRow.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(InstanceData->StartRow.RowName);
	return FText::Format(INVTEXT("Play Dialogue ({0})"), RowText);
}
#endif
