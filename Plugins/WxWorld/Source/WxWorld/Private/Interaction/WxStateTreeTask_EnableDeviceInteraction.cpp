// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_EnableDeviceInteraction.h"

#include "GameFramework/Actor.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_EnableDeviceInteraction::FWxStateTreeTask_EnableDeviceInteraction()
{
	// 같은 상태가 재선택돼도 이미 적용된 값이라 다시 쓸 것이 없다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_EnableDeviceInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr)
	{
		// 발행은 상호작용을 받은 그 순간, 즉 트리 틱 밖에서 일어난다. 그래서 발행자만이 아니라 지금의 실행 컨텍스트를 약참조로 함께 남긴다.
		FWxGimmickDeviceBinding Binding;
		Binding.Dispatcher = Instance.OnInteracted;
		Binding.Context = Context.MakeWeakExecutionContext();

		Gimmick->SetDeviceInteractionEnabled(Instance.Role, Instance.bEnable, Instance.Prompt, Binding);
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableDeviceInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText RoleText = FText::FromName(InstanceData->Role);
	if (InstanceData->bEnable && !InstanceData->Prompt.IsEmpty())
	{
		return FText::Format(INVTEXT("\"{1}\" 장치 상호작용 켜기 ({0})"), RoleText, InstanceData->Prompt);
	}

	return FText::Format(INVTEXT("{0} ({1})"), InstanceData->bEnable ? INVTEXT("장치 상호작용 켜기") : INVTEXT("장치 상호작용 끄기"), RoleText);
}
#endif
