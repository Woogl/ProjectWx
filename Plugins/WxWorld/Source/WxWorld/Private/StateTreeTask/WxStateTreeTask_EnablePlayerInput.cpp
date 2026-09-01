// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_EnablePlayerInput.h"

#include "Device/WxDevice.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_EnablePlayerInput::FWxStateTreeTask_EnablePlayerInput()
{
	bShouldCallTick = false;

	// 그 상태의 입력 가용성을 선언하는 상태형 태스크다. 재선택 시 EnterState/ExitState 가 함께 스킵되므로 아래 차단 기록과 해제의 짝도 그대로 유지된다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_EnablePlayerInput::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 아래 어느 조기 완료 경로로 빠지든 ExitState 가 끄지도 않은 입력을 되돌리는 일이 없어야 한다.
	Instance.DisabledPawn = nullptr;
	Instance.DisabledController = nullptr;

	// 장치 상태는 모든 피어에서 실행되므로, 이 장치의 상호작용 당사자만 입력 대상으로 삼는다.
	AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner());
	ACharacter* InteractingCharacter = Device ? Device->GetInteractingCharacter() : nullptr;
	APlayerController* PC = InteractingCharacter ? Cast<APlayerController>(InteractingCharacter->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController() || PC->GetPawn() != InteractingCharacter)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	APawn* Pawn = InteractingCharacter;

	if (Instance.bEnable)
	{
		Pawn->EnableInput(PC);
	}
	else
	{
		Pawn->DisableInput(PC);

		Instance.DisabledPawn = Pawn;
		Instance.DisabledController = PC;
	}

	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_EnablePlayerInput::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
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
