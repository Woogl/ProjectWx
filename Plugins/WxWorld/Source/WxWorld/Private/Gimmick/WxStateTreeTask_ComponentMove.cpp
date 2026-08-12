// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_ComponentMove.h"

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

	// 길이 0 이거나 이미 목표면 애니 없이 즉시 스냅하고 곧바로 완료한다.
	const bool bReachNow = Instance.Duration <= 0.f || Component->GetRelativeLocation().Equals(Instance.TargetLocation);
	if (bReachNow)
	{
		Component->SetRelativeLocation(Instance.TargetLocation);
		return EStateTreeRunStatus::Succeeded;
	}

	// 속도를 시작(현재)→목표 실제 거리/Duration 으로 1회 산출한다(LocalOffset 크기가 아니라 실제 거리라, 목표가 아키타입인 닫기도 0 이 아니다).
	Instance.MoveSpeed = (Instance.TargetLocation - Component->GetRelativeLocation()).Size() / Instance.Duration;

	// Tick 이 고정 속도로 슬라이드하다 도달 시 완료한다.
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

	// 목표는 EnterState 에서 캐시한 값을 쓴다(아키타입 조회를 매 틱 반복하지 않는다).
	const FVector& Target = Instance.TargetLocation;
	FVector NewLocation = Component->GetRelativeLocation();

	// 도달 전까지 EnterState 에서 산출한 고정 속도로 슬라이드한다.
	if (!NewLocation.Equals(Target))
	{
		NewLocation = FMath::VInterpConstantTo(NewLocation, Target, DeltaTime, Instance.MoveSpeed);
		Component->SetRelativeLocation(NewLocation);
	}

	// 도달하면 상태를 완료시킨다.
	return NewLocation.Equals(Target) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ComponentMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 움직일 컴포넌트는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText ComponentText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetComponent)), Formatting);
	if (ComponentText.IsEmpty())
	{
		ComponentText = InstanceData->TargetComponent ? FText::FromString(InstanceData->TargetComponent->GetName()) : INVTEXT("none");
	}

	return FText::Format(INVTEXT("Component Move ({0})"), ComponentText);
}
#endif
