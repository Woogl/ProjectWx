// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_Patrol.h"

#include "AI/WxAIBehaviorComponent.h"
#include "AbilitySystem/Effects/WxEffect_MoveSpeedScale.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.MoveSpeedEffectHandle = FActiveGameplayEffectHandle();

	APawn* Pawn = Instance.AIController ? Instance.AIController->GetPawn() : nullptr;
	const UWxAIBehaviorComponent* AIBehavior = Pawn ? Pawn->FindComponentByClass<UWxAIBehaviorComponent>() : nullptr;

	FVector PatrolDestination;
	if (!AIBehavior || !AIBehavior->GetPatrolDestination(PatrolDestination))
	{
		return EStateTreeRunStatus::Failed;
	}

	Instance.Destination = PatrolDestination;
	Instance.TargetActor = nullptr;

	// MaxWalkSpeed 를 직접 쓰면 MOV 어트리뷰트 콜백과 주인이 겹쳐, 정찰 중 버프가 걸리거나 정찰이 끝날 때 서로의 값을 덮어쓴다.
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (ASC)
	{
		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UWxEffect_MoveSpeedScale::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, Instance.MoveSpeedMultiplier);
			Instance.MoveSpeedEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}
	}

	return Super::EnterState(Context, Transition);
}

void FWxStateTreeTask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 도착·전이·트리 정지 등 어떤 종료 경로에서도 호출되므로, 감속 GE 제거는 여기서 한다.
	if (UAbilitySystemComponent* ASC = Instance.MoveSpeedEffectHandle.GetOwningAbilitySystemComponent())
	{
		ASC->RemoveActiveGameplayEffect(Instance.MoveSpeedEffectHandle);
	}
	Instance.MoveSpeedEffectHandle = FActiveGameplayEffectHandle();

	Super::ExitState(Context, Transition);
}

void FWxStateTreeTask_Patrol::StateCompleted(FStateTreeExecutionContext& Context, const EStateTreeRunStatus CompletionStatus, const FStateTreeActiveStates& CompletedActiveStates) const
{
	// 전이에 끊기거나 이동에 실패하면 커서를 그대로 둬, 다음 진입에서 같은 지점으로 다시 간다.
	if (CompletionStatus != EStateTreeRunStatus::Succeeded)
	{
		return;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const APawn* Pawn = Instance.AIController ? Instance.AIController->GetPawn() : nullptr;
	if (UWxAIBehaviorComponent* AIBehavior = Pawn ? Pawn->FindComponentByClass<UWxAIBehaviorComponent>() : nullptr)
	{
		AIBehavior->AdvancePatrol();
	}
}

#if WITH_EDITOR
FText FWxStateTreeTask_Patrol::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Patrol (x{0})"), FText::AsNumber(InstanceData->MoveSpeedMultiplier));
}
#endif
