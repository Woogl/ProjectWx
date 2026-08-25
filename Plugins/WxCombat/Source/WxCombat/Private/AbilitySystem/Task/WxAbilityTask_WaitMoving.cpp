// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_WaitMoving.h"
#include "GameFramework/Actor.h"

UWxAbilityTask_WaitMoving* UWxAbilityTask_WaitMoving::CreateTask(UGameplayAbility* OwningAbility, float InMinSpeed)
{
	UWxAbilityTask_WaitMoving* Task = NewAbilityTask<UWxAbilityTask_WaitMoving>(OwningAbility);
	Task->MinSpeedSquared = InMinSpeed * InMinSpeed;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_WaitMoving::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	const bool bNowMoving = IsAvatarMoving();
	if (bNowMoving == bIsMoving)
	{
		return;
	}

	bIsMoving = bNowMoving;

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnMovingChanged.Broadcast(bIsMoving);
	}
}

void UWxAbilityTask_WaitMoving::Activate()
{
	Super::Activate();

	// 이미 달리는 중에 발동했을 수 있으므로 첫 통지는 전환을 기다리지 않는다.
	bIsMoving = IsAvatarMoving();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnMovingChanged.Broadcast(bIsMoving);
	}
}

bool UWxAbilityTask_WaitMoving::IsAvatarMoving() const
{
	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		return false;
	}

	// 낙하·점프 중의 수직 속도는 제외한다 — 제자리 점프가 이동으로 잡히면 안 된다.
	return Avatar->GetVelocity().SizeSquared2D() > MinSpeedSquared;
}
