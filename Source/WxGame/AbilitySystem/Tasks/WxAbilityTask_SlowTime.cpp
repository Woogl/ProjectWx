// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/WxAbilityTask_SlowTime.h"

#include "AbilitySystemComponent.h"
#include "Combat/WxTimeDilationSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWxAbilityTask_SlowTime* UWxAbilityTask_SlowTime::CreateTask(UGameplayAbility* OwningAbility, float InTimeDilation, float InDuration)
{
	UWxAbilityTask_SlowTime* Task = NewAbilityTask<UWxAbilityTask_SlowTime>(OwningAbility);
	Task->TimeDilation = InTimeDilation;
	Task->Duration = InDuration;
	return Task;
}

void UWxAbilityTask_SlowTime::OnDestroy(bool bInOwnerFinished)
{
	if (UWxTimeDilationSubsystem* Subsystem = TimeDilationSubsystem.Get())
	{
		Subsystem->RemoveRequest(TimeDilationHandle);
	}
	TimeDilationHandle = 0;
	TimeDilationSubsystem.Reset();

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_SlowTime::Activate()
{
	Super::Activate();

	// TasksComponent 약참조가 풀리면 World가 널이다. 경과 시간을 못 재면 딜레이션을 걷을 수도 없으므로 걸기 전에 접는다.
	UWorld* World = GetWorld();
	if (!World)
	{
		EndTask();
		return;
	}

	if (AbilitySystemComponent.IsValid() && AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		if (UWxTimeDilationSubsystem* Subsystem = World->GetSubsystem<UWxTimeDilationSubsystem>())
		{
			TimeDilationSubsystem = Subsystem;
			TimeDilationHandle = Subsystem->AddRequest(TimeDilation);
		}
	}

	// 순정 WaitDelay처럼 타이머 핸들을 보관하지 않는다 — 먼저 끝난 태스크에 도착한 EndTask는 무시된다.
	FTimerHandle TimerHandle;
	if (Duration > 0.f)
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &UWxAbilityTask_SlowTime::EndTask, Duration, false);
	}
	else if (Duration == 0.f)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UWxAbilityTask_SlowTime::EndTask);
	}
}
