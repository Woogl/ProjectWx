// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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
	UWorld* World = GetWorld();
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

	// 순정 WaitDelay처럼 핸들을 들지 않는다 — 먼저 끝난 태스크에 도착한 EndTask는 무시된다.
	FTimerHandle TimerHandle;
	if (Duration > 0.f)
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &UWxAbilityTask_SlowTime::EndTask, Duration, false);
	}
	else
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UWxAbilityTask_SlowTime::EndTask);
	}
}
