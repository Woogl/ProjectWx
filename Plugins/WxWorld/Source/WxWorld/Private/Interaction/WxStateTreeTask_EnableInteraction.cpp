// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_EnableInteraction.h"

#include "Device/WxDevice.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

FWxStateTreeTask_EnableInteraction::FWxStateTreeTask_EnableInteraction()
{
	// 진입에서 끝나는 일이라 볼 것이 없다.
	bShouldCallTick = false;

	// 같은 상태가 재선택돼도 이미 적용된 값이라 다시 쓸 것이 없다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 프롬프트·발행 자리까지 담아야 해서 계약이 아니라 장치로 직접 부른다 — 그러지 않으면 공용 계약이 StateTree 를 떠안는다.
	AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner());
	if (!Device)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Enable Interaction: 오너 %s 가 장치가 아니라 상호작용을 여닫을 수 없음."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

		return EStateTreeRunStatus::Succeeded;
	}

	// 발행은 상호작용을 받은 그 순간, 즉 트리 틱 밖에서 일어난다. 그래서 발행자만이 아니라 지금의 실행 컨텍스트를 약참조로 함께 남긴다.
	FWxDeviceInteractionBinding Binding;
	Binding.Prompt = Instance.Prompt;
	Binding.Dispatcher = Instance.OnInteracted;
	Binding.Context = Context.MakeWeakExecutionContext();

	// 콜리전은 건드리지 않으므로 대상 메시의 설정은 보존된다.
	Device->SetInteractionBinding(Instance.bEnable, Binding);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	if (InstanceData->bEnable && !InstanceData->Prompt.IsEmpty())
	{
		return FText::Format(INVTEXT("\"{0}\" 상호작용 켜기"), InstanceData->Prompt);
	}

	return InstanceData->bEnable ? INVTEXT("상호작용 켜기") : INVTEXT("상호작용 끄기");
}
#endif
