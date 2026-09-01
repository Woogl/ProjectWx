// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UWxAbilityTask_SlowTime* UWxAbilityTask_SlowTime::CreateTask(UGameplayAbility* OwningAbility, float InTimeDilation, float InDuration)
{
	UWxAbilityTask_SlowTime* Task = NewAbilityTask<UWxAbilityTask_SlowTime>(OwningAbility);
	Task->TimeDilation = InTimeDilation;
	Task->Duration = InDuration;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_SlowTime::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	const UWorld* World = GetWorld();
	if (!World)
	{
		EndTask();
		return;
	}

	if (World->GetRealTimeSeconds() - StartRealTimeSeconds >= Duration)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnFinished.Broadcast();
		}

		EndTask();
	}
}

void UWxAbilityTask_SlowTime::OnDestroy(bool bInOwnerFinished)
{
	// 애초에 걸지 못한 머신은 AppliedDilation이 0이라 이 조건에서 함께 걸러진다.
	if (AppliedDilation > 0.f && FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(this), AppliedDilation))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_SlowTime::Activate()
{
	Super::Activate();

	// TasksComponent 약참조가 풀리면 World가 널이다. 경과 시간을 못 재면 딜레이션을 걷을 수도 없으므로 걸기 전에 접는다.
	const UWorld* World = GetWorld();
	if (!World)
	{
		EndTask();
		return;
	}

	if (AbilitySystemComponent.IsValid() && AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		UGameplayStatics::SetGlobalTimeDilation(this, TimeDilation);

		// 엔진이 Min/MaxGlobalTimeDilation으로 클램프하므로, 해제 때 비교하려면 요청값이 아니라 실제로 박힌 값을 들고 있어야 한다.
		AppliedDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	}

	StartRealTimeSeconds = World->GetRealTimeSeconds();
}
