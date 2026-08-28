// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_SendEvent.h"

#include "Components/ChildActorComponent.h"
#include "Device/WxDevice.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxWorldModule.h"

FWxStateTreeTask_SendEvent::FWxStateTreeTask_SendEvent()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_SendEvent::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 보내지 않는다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AWxDevice* Owner = Cast<AWxDevice>(Context.GetOwner());
	if (!Owner)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Send Event: 오너 %s 가 장치가 아님."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

		return EStateTreeRunStatus::Succeeded;
	}

	// 상태를 움직이는 것은 권위 트리뿐이다(받는 장치도 한 번 더 가린다). 클라에서 먼저 걸러 아래 저작 진단이 피어마다 반복되지 않게 한다.
	if (!Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 당사자를 그대로 넘겨 받는 장치의 몽타주·GE 태스크가 같은 캐릭터를 대상으로 삼게 한다.
	ACharacter* Interactor = Owner->GetInteractingCharacter();

	if (Instance.TargetKind == EWxDeviceEventTarget::Linked)
	{
		// 선언해 놓고 배선이 비면 보낼 곳이 없다 — 조용히 통과하면 저작 실수가 드러나지 않는다.
		if (Instance.LinkedDevices.IsEmpty())
		{
			UE_LOG(LogWxWorld, Warning, TEXT("Send Event: %s 의 연결 장치가 비어 보낼 곳이 없음."), *GetNameSafe(Owner));

			return EStateTreeRunStatus::Succeeded;
		}

		for (AWxDevice* LinkedDevice : Instance.LinkedDevices)
		{
			if (IsValid(LinkedDevice))
			{
				LinkedDevice->NotifyDeviceInteracted(Interactor, Instance.Event, Instance.Payload);
			}
		}

		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.ChildDevice.Name.IsNone())
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Send Event: %s 가 내장 장치를 지목하지 않아 보낼 곳이 없음."), *GetNameSafe(Owner));

		return EStateTreeRunStatus::Succeeded;
	}

	const UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>(Instance.ChildDevice.Resolve(Owner));
	AWxDevice* ChildDevice = ChildActorComponent ? Cast<AWxDevice>(ChildActorComponent->GetChildActor()) : nullptr;
	if (!ChildDevice)
	{
		UE_LOG(LogWxWorld, Error, TEXT("Send Event: %s 에서 내장 장치 '%s' 를 찾지 못했다."), *GetNameSafe(Owner), *Instance.ChildDevice.Name.ToString());

		return EStateTreeRunStatus::Failed;
	}

	ChildDevice->NotifyDeviceInteracted(Interactor, Instance.Event, Instance.Payload);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SendEvent::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	FText TargetText = InstanceData->ChildDevice.Name.IsNone() ? INVTEXT("대상 없음") : FText::FromName(InstanceData->ChildDevice.Name);
	if (InstanceData->TargetKind == EWxDeviceEventTarget::Linked)
	{
		// 배치 대상은 바인딩이라 편집 중엔 배열이 비어 있다 — 무엇에 걸었는지를 보여야 배선을 빠뜨린 노드가 눈에 띈다.
		TargetText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, LinkedDevices)), Formatting);
		if (TargetText.IsEmpty())
		{
			TargetText = INVTEXT("대상 없음");
		}
	}

	FText EventText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Event)), Formatting);
	if (EventText.IsEmpty())
	{
		EventText = InstanceData->Event.IsValid() ? FText::FromName(InstanceData->Event.GetTagName()) : INVTEXT("태그 없음");
	}

	return FText::Format(INVTEXT("{0} 에 이벤트 보내기 ({1})"), TargetText, EventText);
}
#endif
