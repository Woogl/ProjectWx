// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_TriggerLinkedDevices.h"

#include "Device/WxDevice.h"
#include "Device/WxDeviceStateTreeComponent.h"
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
	if (UWxDeviceStateTreeComponent::IsRestoring(Context, Transition))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AWxDevice* Owner = Cast<AWxDevice>(Context.GetOwner());
	if (!Owner)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Trigger Linked Devices: 오너 %s 가 장치가 아님."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

		return EStateTreeRunStatus::Succeeded;
	}

	// 받는 장치도 권위를 가리지만, 클라에서 먼저 걸러 아래 저작 진단이 피어마다 반복되지 않게 한다.
	if (!Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Owner->LinkedDevices.IsEmpty())
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Trigger Linked Devices: %s 의 연결 장치가 비어 보낼 곳이 없음."), *GetNameSafe(Owner));

		return EStateTreeRunStatus::Succeeded;
	}

	for (AWxDevice* LinkedDevice : Owner->LinkedDevices)
	{
		if (IsValid(LinkedDevice))
		{
			LinkedDevice->NotifyDeviceInteracted(Owner->GetInteractingCharacter(), Owner, Owner->GetSelectedOptionValue());
		}
	}

	return EStateTreeRunStatus::Succeeded;
}
