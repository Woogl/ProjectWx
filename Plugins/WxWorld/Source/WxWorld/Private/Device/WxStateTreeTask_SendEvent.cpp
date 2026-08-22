// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_SendEvent.h"

#include "Components/ChildActorComponent.h"
#include "Device/WxDevice.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"
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
		UE_LOG(LogWxWorld, Warning, TEXT("Send Event: 오너 %s 가 장치가 아니라 대상도 당사자도 읽을 곳이 없음."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

		return EStateTreeRunStatus::Succeeded;
	}

	// 상태를 움직이는 것은 권위 트리뿐이다(받는 장치도 한 번 더 가린다). 클라에서 먼저 걸러 아래 저작 진단이 피어마다 반복되지 않게 한다.
	if (!Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 비운 칸은 오너가 든 값으로 채운다 — 배치마다 달라지는 값은 액터에, 에셋이 정하는 값은 태스크에 실린다.
	const FGameplayTag EventTag = Instance.Event.IsValid() ? Instance.Event : Owner->TriggerEvent;

	// 당사자를 그대로 넘겨 받는 장치의 이동·몽타주 태스크가 같은 캐릭터를 대상으로 삼게 한다.
	ACharacter* Interactor = Owner->GetInteractingCharacter();

	if (Instance.ChildDevice.Name.IsNone())
	{
		// 오너가 아는 상대 전부. 지목한 장치도 민 장치도 없으면 무동작으로 통과한다 — 아무것도 밀지 않는 발동 장치도 성립한다.
		TArray<AWxDevice*, TInlineAllocator<4>> Targets(Owner->LinkedDevices);
		Targets.AddUnique(Owner->GetInstigatorDevice());

		for (AWxDevice* Device : Targets)
		{
			if (IsValid(Device))
			{
				Device->NotifyDeviceInteracted(Interactor, EventTag, Instance.Payload, Owner);
			}
		}

		return EStateTreeRunStatus::Succeeded;
	}

	const UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>(Instance.ChildDevice.Resolve(Owner));
	AWxDevice* ChildDevice = ChildActorComponent ? Cast<AWxDevice>(ChildActorComponent->GetChildActor()) : nullptr;
	if (!ChildDevice)
	{
		UE_LOG(LogWxWorld, Error, TEXT("Send Event: %s 에서 내장 장치 '%s' 를 찾지 못했다."), *GetNameSafe(Owner), *Instance.ChildDevice.Name.ToString());

		return EStateTreeRunStatus::Failed;
	}

	ChildDevice->NotifyDeviceInteracted(Interactor, EventTag, Instance.Payload, Owner);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SendEvent::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText TargetText = InstanceData->ChildDevice.Name.IsNone() ? INVTEXT("연결 장치") : FText::FromName(InstanceData->ChildDevice.Name);
	const FText EventText = InstanceData->Event.IsValid() ? FText::FromName(InstanceData->Event.GetTagName()) : INVTEXT("Trigger Event");

	return FText::Format(INVTEXT("{0} 에 이벤트 보내기 ({1})"), TargetText, EventText);
}
#endif
