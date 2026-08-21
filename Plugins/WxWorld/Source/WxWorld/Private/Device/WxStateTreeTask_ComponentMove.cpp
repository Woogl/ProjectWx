// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_ComponentMove.h"

#include "Components/SceneComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

FWxStateTreeTask_ComponentMove::FWxStateTreeTask_ComponentMove()
{
	// 그 상태의 목표 포즈를 선언하는 상태형 태스크다. 재선택마다 다시 진입하면 이동 중이던 슬라이드가 끊기고 속도가 재산출된다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_ComponentMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	if (!Component)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 기준 포즈 = 컴포넌트 아키타입(BP/CDO 오서링)의 상대 위치. 런타임 위치가 어디든 안정적 앵커다.
	// 아키타입 조회가 상수 시간이 아니라 여기서 1회만 구해 인스턴스 데이터에 캐시하고, Tick 은 이 값을 읽기만 한다.
	const USceneComponent* Archetype = Cast<USceneComponent>(Component->GetArchetype());
	const FVector Anchor = Archetype ? Archetype->GetRelativeLocation() : Component->GetRelativeLocation();
	Instance.TargetLocation = Anchor + Instance.LocalOffset;

	const bool bReachNow = Instance.Duration <= 0.f || Component->GetRelativeLocation().Equals(Instance.TargetLocation);
	if (bReachNow)
	{
		Component->SetRelativeLocation(Instance.TargetLocation);
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.MoveSpeed = (Instance.TargetLocation - Component->GetRelativeLocation()).Size() / Instance.Duration;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_ComponentMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	if (!Component)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector& Target = Instance.TargetLocation;
	FVector NewLocation = Component->GetRelativeLocation();

	if (!NewLocation.Equals(Target))
	{
		NewLocation = FMath::VInterpConstantTo(NewLocation, Target, DeltaTime, Instance.MoveSpeed);
		Component->SetRelativeLocation(NewLocation);
	}

	return NewLocation.Equals(Target) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ComponentMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 움직일 컴포넌트는 보통 바인딩이라 런타임 포인터가 비어 있다.
	FText ComponentText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetComponent)), Formatting);
	if (ComponentText.IsEmpty())
	{
		ComponentText = InstanceData->TargetComponent ? FText::FromString(InstanceData->TargetComponent->GetName()) : INVTEXT("none");
	}

	return FText::Format(INVTEXT("컴포넌트 이동 ({0})"), ComponentText);
}
#endif
