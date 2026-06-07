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

	if (GetWorld()->GetRealTimeSeconds() - StartRealTimeSeconds >= Duration)
	{
		OnFinished.Broadcast();
		EndTask();
	}
}

void UWxAbilityTask_SlowTime::OnDestroy(bool bInOwnerFinished)
{
	UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(this, 1.f);

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_SlowTime::Activate()
{
	Super::Activate();

	UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(this, TimeDilation);

	StartRealTimeSeconds = GetWorld()->GetRealTimeSeconds();
}
