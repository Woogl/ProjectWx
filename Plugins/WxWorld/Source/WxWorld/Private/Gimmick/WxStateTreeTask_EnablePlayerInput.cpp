// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_EnablePlayerInput.h"

#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_EnablePlayerInput::FWxStateTreeTask_EnablePlayerInput()
{
	// 입력을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;

	// 그 상태의 입력 가용성을 선언하는 상태형 태스크다. 재선택 시 EnterState/ExitState 가 함께 스킵되므로 아래 차단 기록과 해제의 짝도 그대로 유지된다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_EnablePlayerInput::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 차단 기록을 비운 상태로 시작한다. 아래 어느 조기 완료 경로로 빠지든 ExitState 가 끄지도 않은 입력을 되돌리는 일이 없어야 한다.
	Instance.DisabledPawn = nullptr;
	Instance.DisabledController = nullptr;

	// 입력은 로컬에만 존재하므로 이 머신의 로컬 플레이어를 토글한다. PC/Pawn 이 없으면(예: 데디 서버) 노옵.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	APlayerController* PC = GEngine ? GEngine->GetFirstLocalPlayerController(Owner ? Owner->GetWorld() : nullptr) : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.bEnable)
	{
		Pawn->EnableInput(PC);
	}
	else
	{
		Pawn->DisableInput(PC);

		// 실제로 끈 대상을 그때그때 기록해 둔다 — ExitState 는 이 기록만 보고 되돌리므로, 그 사이 폰이 소멸·언포제스돼도 엉뚱한 대상을 켜지 않는다.
		Instance.DisabledPawn = Pawn;
		Instance.DisabledController = PC;
	}

	// 토글은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_EnablePlayerInput::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// EnterState 에서 끈 입력을 되돌린다. 복구를 "다음 상태에 Enable Player Input(true) 가 배선되어 있을 것"이라는 에셋 규약에만 맡기면,
	// 연출 중 기믹 액터/셀이 사라져 ST 가 멈추거나 디자이너가 다음 상태에 토글을 빠뜨렸을 때 입력이 꺼진 채 남아 소프트락이 된다.
	// 켜는 노드(bEnable)는 기록을 남기지 않으므로 여기서 되돌릴 것도 없다.
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	APawn* Pawn = Instance.DisabledPawn.Get();
	APlayerController* PC = Instance.DisabledController.Get();
	if (Pawn && PC)
	{
		Pawn->EnableInput(PC);
	}

	Instance.DisabledPawn = nullptr;
	Instance.DisabledController = nullptr;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnablePlayerInput::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return InstanceData->bEnable ? INVTEXT("플레이어 입력 켜기") : INVTEXT("플레이어 입력 끄기");
}
#endif
