// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_Wander.h"

#include "AbilitySystem/Effects/WxEffect_MoveSpeedScale.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_Wander::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.ElapsedTime = 0.f;
	Instance.MoveSpeedEffectHandle = FActiveGameplayEffectHandle();

	AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 등분한 구간마다 한 번씩 시도해야 좁은 범위에서도 같은 방향만 반복해 재보지 않는다.
	constexpr int32 SectorCount = 8;
	TArray<int32, TInlineAllocator<SectorCount>> RemainingSectors;
	for (int32 Index = 0; Index < SectorCount; ++Index)
	{
		RemainingSectors.Add(Index);
	}

	const UPawnMovementComponent* Movement = Pawn->GetMovementComponent();
	if (!Movement)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 감속 GE 는 방향을 고른 뒤 부여되므로 지금 최대 속도는 아직 평상시 값이다.
	const float TravelDistance = Movement->GetMaxSpeed() * Instance.MoveSpeedMultiplier * Instance.Duration;

	// 걸어갈 거리가 0이면 길이 0 레이가 막힘으로 오지 않아 후보가 전부 무검증 통과한다.
	// 지금 못 움직인다고 무해한 것이 아니다 — 배회가 끝나기 전에 속박이 풀리면 검증되지 않은 방향으로 걸어 나간다.
	if (FMath::IsNearlyZero(TravelDistance))
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector NavStart = Pawn->GetNavAgentLocation();

	// 범위를 뒤집어 넣으면 폭이 음수가 되어 같은 부채꼴을 반대로 훑을 뿐이라 따로 바로잡지 않는다.
	const float SectorSize = (Instance.MaxAngle - Instance.MinAngle) / SectorCount;

	bool bFoundDirection = false;
	while (RemainingSectors.Num() > 0)
	{
		const int32 PickedSlot = FMath::RandRange(0, RemainingSectors.Num() - 1);
		const float Angle = Instance.MinAngle + (RemainingSectors[PickedSlot] + FMath::FRand()) * SectorSize;
		const FVector Candidate = FRotator(0.f, AIController->GetControlRotation().Yaw + Angle, 0.f).Vector();
		RemainingSectors.RemoveAtSwap(PickedSlot);

		// 내비 데이터가 아예 없어도 막힘으로 온다.
		FVector HitLocation;
		if (UNavigationSystemV1::NavigationRaycast(Pawn, NavStart, NavStart + Candidate * TravelDistance, HitLocation, nullptr, AIController))
		{
			continue;
		}

		Instance.MoveDirection = Candidate;
		bFoundDirection = true;
		break;
	}

	if (!bFoundDirection)
	{
		return EStateTreeRunStatus::Failed;
	}

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

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_Wander::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	Instance.ElapsedTime += DeltaTime;
	if (Instance.ElapsedTime >= Instance.Duration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 속도는 감속 GE 가 낮춘 MOV → MaxWalkSpeed 가 제어하므로 입력 스케일은 1.0 으로 넣는다.
	Pawn->AddMovementInput(Instance.MoveDirection, 1.f);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_Wander::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 완료·전이·트리 정지 등 어떤 종료 경로에서도 호출되므로, 감속 GE 제거는 여기서 한다.
	if (UAbilitySystemComponent* ASC = Instance.MoveSpeedEffectHandle.GetOwningAbilitySystemComponent())
	{
		ASC->RemoveActiveGameplayEffect(Instance.MoveSpeedEffectHandle);
	}
	Instance.MoveSpeedEffectHandle = FActiveGameplayEffectHandle();
}

#if WITH_EDITOR
FText FWxStateTreeTask_Wander::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("배회 ({0}초, x{1})"), FText::AsNumber(InstanceData->Duration), FText::AsNumber(InstanceData->MoveSpeedMultiplier));
}
#endif
