// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_SpawnActor.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_SpawnActor::FWxStateTreeTask_SpawnActor()
{
	// 완료 없이 머무는 태스크다. 재선택마다 재진입하면 ExitState 의 bDestroyOnExit 가 스폰체를 전멸시키고 누적기가 리셋돼 스폰 주기가 끊긴다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_SpawnActor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 스폰체는 Transient 라 복원할 포즈가 없다. 누적기를 Interval 로 채워 진입 첫 틱에 즉시 1회 스폰한다(이후 반복 여부는 Tick 의 주기 처리가 정한다).
	Instance.TimeSinceLastSpawn = Instance.Interval;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_SpawnActor::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 스폰은 서버 권위 사건이다. 클라는 복제로 스폰체를 추종하므로 진행시키지 않는다.
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Running;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	Instance.TimeSinceLastSpawn += DeltaTime;
	if (Instance.TimeSinceLastSpawn < Instance.Interval)
	{
		// 아직 간격에 못 미쳤다. 머무는 태스크라 완료하지 않는다.
		return EStateTreeRunStatus::Running;
	}
	// Interval 양수면 한 주기 차감해 반복 스폰하고, 0(일회성)이면 누적기를 음의 극값으로 묶어 다시 임계에 도달하지 못하게 해 1회만 스폰한다.
	Instance.TimeSinceLastSpawn = Instance.Interval > 0.f ? Instance.TimeSinceLastSpawn - Instance.Interval : -TNumericLimits<float>::Max();

	UWorld* World = Owner->GetWorld();
	if (!World || !Instance.ActorClass)
	{
		return EStateTreeRunStatus::Running;
	}

	// 스폰 위치·회전·크기는 LocalSpawnTransform 을 오너 월드 트랜스폼에 합성해 정한다(Identity 면 오너 트랜스폼 그대로).
	const FTransform SpawnTransform = Instance.LocalSpawnTransform * Owner->GetActorTransform();
	AActor* Spawned = World->SpawnActorDeferred<AActor>(Instance.ActorClass, SpawnTransform, Owner, Owner->GetInstigator(), Instance.SpawnCollisionHandlingOverride);
	if (!Spawned)
	{
		return EStateTreeRunStatus::Running;
	}

	// Lifetime 양수면 그만큼 살다 자동 파괴된다(0 이하면 직접 파괴/이탈 정리에 맡김).
	if (Instance.Lifetime > 0.f)
	{
		Spawned->SetLifeSpan(Instance.Lifetime);
	}
	Spawned->FinishSpawning(SpawnTransform);

	// 자동 파괴(수명 만료)된 항목은 pending-kill 이라 IsValid=false 로 걸러 목록을 유계로 유지한다.
	for (int32 Index = Instance.SpawnedActors.Num() - 1; Index >= 0; --Index)
	{
		if (!IsValid(Instance.SpawnedActors[Index]))
		{
			Instance.SpawnedActors.RemoveAtSwap(Index);
		}
	}
	Instance.SpawnedActors.Add(Spawned);

	// 완료 전이 없는 머무는 상태다. 항상 Running 을 유지한다.
	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_SpawnActor::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 스폰·파괴는 서버 권위 사건이라 권위 측만 처리한다(클라는 SpawnedActors 가 비어 있고 복제로 추종).
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// bDestroyOnExit 면 상태를 떠날 때(Active→Disabled 등) 추적 중인 스폰체를 전부 제거한다. 끄면 남겨 각자 Lifetime 으로 끝까지 살다 자동 파괴되게 둔다.
	if (Instance.bDestroyOnExit)
	{
		for (const TObjectPtr<AActor>& Spawned : Instance.SpawnedActors)
		{
			if (IsValid(Spawned))
			{
				Spawned->Destroy();
			}
		}
	}
	Instance.SpawnedActors.Reset();
}

#if WITH_EDITOR
FText FWxStateTreeTask_SpawnActor::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Spawn Actor ({0})"),
		InstanceData->ActorClass ? FText::FromString(InstanceData->ActorClass->GetName()) : INVTEXT("none"));
}
#endif
