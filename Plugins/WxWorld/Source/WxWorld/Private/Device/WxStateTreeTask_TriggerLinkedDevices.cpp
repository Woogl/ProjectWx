// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_TriggerLinkedDevices.h"

#include "Device/WxDevice.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

FWxStateTreeTask_TriggerLinkedDevices::FWxStateTreeTask_TriggerLinkedDevices()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_TriggerLinkedDevices::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 통지하지 않는다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AWxDevice* Owner = Cast<AWxDevice>(Context.GetOwner());
	if (!Owner)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Trigger Linked Devices: 오너 %s 가 장치가 아니라 지목을 읽을 곳이 없음."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

		return EStateTreeRunStatus::Succeeded;
	}

	// 상태를 움직이는 것은 권위 트리뿐이다(받는 장치도 한 번 더 가린다).
	if (!Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 당사자를 그대로 넘겨 받는 장치의 이동·몽타주 태스크가 같은 캐릭터를 대상으로 삼게 한다.
	ACharacter* Interactor = Owner->GetInteractingCharacter();
	for (AWxDevice* Device : Owner->LinkedDevices)
	{
		if (IsValid(Device))
		{
			Device->NotifyDeviceInteracted(Interactor, Owner->TriggerEvent);
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_TriggerLinkedDevices::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return INVTEXT("연결 장치 발동");
}
#endif
