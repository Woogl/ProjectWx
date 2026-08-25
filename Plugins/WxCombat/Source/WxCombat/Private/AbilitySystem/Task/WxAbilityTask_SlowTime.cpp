// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"

#include "Engine/World.h"
#include "Time/WxTimeDilationComponent.h"

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
	UWxTimeDilationComponent::ClearGlobalTimeDilationAuthoritative(this);

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

	UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(this, TimeDilation);

	StartRealTimeSeconds = World->GetRealTimeSeconds();
}
