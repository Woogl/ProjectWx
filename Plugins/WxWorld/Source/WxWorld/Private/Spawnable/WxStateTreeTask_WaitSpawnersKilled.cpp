// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_WaitSpawnersKilled.h"

#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxSpawnerLocatorUtils.h"
#include "WxWorldModule.h"

namespace
{
	constexpr float SpawnerKilledCheckInterval = 0.25f;
}

FWxStateTreeTask_WaitSpawnersKilled::FWxStateTreeTask_WaitSpawnersKilled()
{
	// 명시적으로 요청한 저주기 틱이 이 태스크의 스케줄을 결정한다.
	bConsideredForScheduling = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;

	// 처치 대기 중 같은 상태가 재선택되어도 런타임 캐시와 판정 주기를 다시 만들 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitSpawnersKilled::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.ResolvedSpawners.Reset();
	Instance.ResolvedSpawners.SetNum(Instance.Spawners.Num());

	// 지정이 없거나 빈 로케이터가 섞이면 완료될 수 없는 잘못된 조립이다.
	if (Instance.Spawners.IsEmpty())
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Wait Spawners Killed: 판정할 스포너가 지정되지 않음."));
	}
	for (const FUniversalObjectLocator& Locator : Instance.Spawners)
	{
		if (Locator.IsEmpty())
		{
			UE_LOG(LogWxWorld, Warning, TEXT("Wait Spawners Killed: 빈 로케이터 항목이 있음(지정 %d개)."), Instance.Spawners.Num());
			break;
		}
	}

	UObject* Owner = Context.GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	int32 ResolvedCount = 0;
	if (AreAllSpawnersKilled(Instance, World, ResolvedCount))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 하나도 해석되지 않은 채 대기에 들어가면, 뒤에 스텝이 안 끝나도 증상만 남고 원인이 안 보인다.
	if (ResolvedCount == 0)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Wait Spawners Killed: 해석된 스포너가 없음(지정 %d개)."), Instance.Spawners.Num());
	}

	Instance.RemainingCheckTime = SpawnerKilledCheckInterval;
	Instance.ScheduledTickHandle = Context.AddScheduledTickRequest(FStateTreeScheduledTick::MakeCustomTickRate(Instance.RemainingCheckTime));

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitSpawnersKilled::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.RemainingCheckTime -= DeltaTime;

	if (Instance.RemainingCheckTime > 0.f)
	{
		Context.UpdateScheduledTickRequest(Instance.ScheduledTickHandle, FStateTreeScheduledTick::MakeCustomTickRate(Instance.RemainingCheckTime));
		return EStateTreeRunStatus::Running;
	}

	UObject* Owner = Context.GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	int32 ResolvedCount = 0;
	if (AreAllSpawnersKilled(Instance, World, ResolvedCount))
	{
		Context.RemoveScheduledTickRequest(Instance.ScheduledTickHandle);
		Instance.ScheduledTickHandle = {};
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.RemainingCheckTime = SpawnerKilledCheckInterval;
	Context.UpdateScheduledTickRequest(Instance.ScheduledTickHandle, FStateTreeScheduledTick::MakeCustomTickRate(Instance.RemainingCheckTime));

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitSpawnersKilled::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.ScheduledTickHandle.IsValid())
	{
		Context.RemoveScheduledTickRequest(Instance.ScheduledTickHandle);
		Instance.ScheduledTickHandle = {};
	}
}

#if WITH_EDITOR
EDataValidationResult FWxStateTreeTask_WaitSpawnersKilled::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	const FInstanceDataType* InstanceData = CompileContext.GetInstanceDataView().GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FWxSpawnerLocatorUtils::ValidateSpawners(CompileContext, InstanceData->Spawners);
}

FText FWxStateTreeTask_WaitSpawnersKilled::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("스포너 처치 대기 ({0})"), FWxLocatorUtils::GetDisplayNamesText(InstanceData->Spawners));
}
#endif

AWxSpawner* FWxStateTreeTask_WaitSpawnersKilled::ResolveSpawner(
	const FUniversalObjectLocator& Locator,
	TWeakObjectPtr<AWxSpawner>& CachedSpawner,
	UWorld* World)
{
	if (!World)
	{
		CachedSpawner.Reset();
		return nullptr;
	}

	AWxSpawner* Spawner = CachedSpawner.Get();
	if (Spawner && Spawner->GetWorld() == World)
	{
		return Spawner;
	}

	CachedSpawner.Reset();
	for (TActorIterator<AWxSpawner> It(World); It; ++It)
	{
		AWxSpawner* Candidate = *It;
		if (Locator.SyncFind(Candidate) == Candidate)
		{
			CachedSpawner = Candidate;
			return Candidate;
		}
	}

	return nullptr;
}

bool FWxStateTreeTask_WaitSpawnersKilled::AreAllSpawnersKilled(FInstanceDataType& Instance, UWorld* World, int32& OutResolvedCount)
{
	OutResolvedCount = 0;
	if (Instance.Spawners.IsEmpty() || Instance.ResolvedSpawners.Num() != Instance.Spawners.Num())
	{
		return false;
	}

	bool bAllKilled = true;
	for (int32 Index = 0; Index < Instance.Spawners.Num(); ++Index)
	{
		AWxSpawner* Spawner = ResolveSpawner(Instance.Spawners[Index], Instance.ResolvedSpawners[Index], World);
		if (!Spawner)
		{
			bAllKilled = false;
			continue;
		}

		++OutResolvedCount;
		if (!Spawner->IsKilled())
		{
			bAllKilled = false;
		}
	}

	return bAllKilled;
}
