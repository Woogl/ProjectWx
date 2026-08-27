// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_WaitSpawnersKilled.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxSpawnerLocatorUtils.h"
#include "WxStateTreeWaitRegistry.h"
#include "WxWorldModule.h"

namespace
{
	TWxStateTreeWaitRegistry<TArray<FUniversalObjectLocator>> SpawnersKilledWaits;
}

FWxStateTreeTask_WaitSpawnersKilled::FWxStateTreeTask_WaitSpawnersKilled()
{
	// 완료를 통보로 받으므로 볼 것이 없다.
	bShouldCallTick = false;

	// 처치 대기 중 같은 상태가 재선택되어도 등록을 다시 할 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

void FWxStateTreeTask_WaitSpawnersKilled::NotifySpawnerKilled(const AWxSpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	SpawnersKilledWaits.FinishMatching(Spawner->GetWorld(), &FWxStateTreeTask_WaitSpawnersKilled::AreAllSpawnersKilled);
}

EStateTreeRunStatus FWxStateTreeTask_WaitSpawnersKilled::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

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

	if (AreAllSpawnersKilled(Instance.Spawners, Context.GetOwner()))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 하나도 해석되지 않은 채 대기에 들어가면, 뒤에 스텝이 안 끝나도 증상만 남고 원인이 안 보인다.
	bool bAnyResolved = false;
	for (const FUniversalObjectLocator& Locator : Instance.Spawners)
	{
		if (Locator.SyncFind(Context.GetOwner()))
		{
			bAnyResolved = true;
			break;
		}
	}

	if (!bAnyResolved)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Wait Spawners Killed: 해석된 스포너가 없음(지정 %d개)."), Instance.Spawners.Num());
	}

	Instance.WaitHandle = SpawnersKilledWaits.Add(Context, Instance.Spawners);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitSpawnersKilled::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	SpawnersKilledWaits.Remove(Instance.WaitHandle);
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

bool FWxStateTreeTask_WaitSpawnersKilled::AreAllSpawnersKilled(const TArray<FUniversalObjectLocator>& Spawners, UObject* Owner)
{
	if (Spawners.IsEmpty())
	{
		return false;
	}

	for (const FUniversalObjectLocator& Locator : Spawners)
	{
		const AWxSpawner* Spawner = Cast<AWxSpawner>(Locator.SyncFind(Owner));
		if (!Spawner || !Spawner->IsKilled())
		{
			return false;
		}
	}

	return true;
}
