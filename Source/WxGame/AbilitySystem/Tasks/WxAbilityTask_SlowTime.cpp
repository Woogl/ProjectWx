// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/WxAbilityTask_SlowTime.h"

#include "AbilitySystemComponent.h"
#include "Combat/WxTimeDilationSubsystem.h"
#include "Engine/World.h"

UWxAbilityTask_SlowTime* UWxAbilityTask_SlowTime::CreateTask(UGameplayAbility* OwningAbility, float InTimeDilation)
{
	UWxAbilityTask_SlowTime* Task = NewAbilityTask<UWxAbilityTask_SlowTime>(OwningAbility);
	Task->TimeDilation = InTimeDilation;
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

	// TasksComponent 약참조가 풀리면 World가 널이다.
	UWorld* World = GetWorld();
	if (World && AbilitySystemComponent.IsValid() && AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		if (UWxTimeDilationSubsystem* Subsystem = World->GetSubsystem<UWxTimeDilationSubsystem>())
		{
			TimeDilationSubsystem = Subsystem;
			TimeDilationHandle = Subsystem->AddRequest(TimeDilation);
		}
	}
}
