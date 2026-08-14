// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_WaitSpawnersKilled.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "System/WxSpawnerLibrary.h"
#include "WxWorldModule.h"

namespace
{
	/** 처치를 기다리는 노드 하나. 완료 통보는 상태가 살아 있는 동안에만 유효한 약한 실행 컨텍스트로 보낸다. */
	struct FWxSpawnersKilledWait
	{
		int32 Handle = INDEX_NONE;
		TArray<FUniversalObjectLocator> Spawners;
		FStateTreeWeakExecutionContext Context;
	};

	/** 비어 있으면 스포너가 처치돼도 하는 일이 없다. */
	TArray<FWxSpawnersKilledWait> SpawnersKilledWaits;

	/** 재사용하지 않으므로 뒤늦은 해제 요청이 엉뚱한 등록을 걷어가지 않는다. */
	int32 NextSpawnersKilledWaitHandle = 0;

	/** 미해석은 판정 불가라 통과시키지 않는다. 지정이 없으면 완료할 근거도 없다. */
	bool AreAllSpawnersKilled(const TArray<FUniversalObjectLocator>& Spawners, UObject* ResolveContext)
	{
		if (Spawners.IsEmpty())
		{
			return false;
		}

		for (const FUniversalObjectLocator& Locator : Spawners)
		{
			const AWxSpawner* Spawner = Cast<AWxSpawner>(Locator.SyncFind(ResolveContext));
			if (!Spawner || !Spawner->IsKilled())
			{
				return false;
			}
		}

		return true;
	}
}

FWxStateTreeTask_WaitSpawnersKilled::FWxStateTreeTask_WaitSpawnersKilled()
{
	// 완료를 통보로 받으므로 볼 것이 없다.
	bShouldCallTick = false;

	// 처치 대기 중 같은 상태가 재선택되어도 등록을 다시 할 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

void FWxStateTreeTask_WaitSpawnersKilled::NotifySpawnerKilled()
{
	for (int32 Index = SpawnersKilledWaits.Num() - 1; Index >= 0; --Index)
	{
		const FWxSpawnersKilledWait& Wait = SpawnersKilledWaits[Index];

		// 트리째 사라진(오너 파괴) 등록은 통보할 곳이 없으므로 이 참에 걷어낸다.
		TStrongObjectPtr<UObject> Owner = Wait.Context.GetOwner();
		if (!Owner)
		{
			SpawnersKilledWaits.RemoveAt(Index);
			continue;
		}

		// 완료한 노드는 상태를 떠나면서 ExitState 에서 스스로 등록을 걷어간다.
		if (AreAllSpawnersKilled(Wait.Spawners, Owner.Get()))
		{
			Wait.Context.FinishTask(EStateTreeFinishTaskType::Succeeded);
		}
	}
}

EStateTreeRunStatus FWxStateTreeTask_WaitSpawnersKilled::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 지정이 없거나 빈 로케이터가 섞이면 완료될 수 없는 잘못된 조립이다. 침묵 대기 대신 경고를 남긴다.
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

	Instance.WaitHandle = NextSpawnersKilledWaitHandle++;

	FWxSpawnersKilledWait& Wait = SpawnersKilledWaits.AddDefaulted_GetRef();
	Wait.Handle = Instance.WaitHandle;
	Wait.Spawners = Instance.Spawners;
	Wait.Context = Context.MakeWeakExecutionContext();

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_WaitSpawnersKilled::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	for (int32 Index = 0; Index < SpawnersKilledWaits.Num(); ++Index)
	{
		if (SpawnersKilledWaits[Index].Handle == Instance.WaitHandle)
		{
			SpawnersKilledWaits.RemoveAt(Index);
			break;
		}
	}
}

#if WITH_EDITOR
EDataValidationResult FWxStateTreeTask_WaitSpawnersKilled::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	const FInstanceDataType* InstanceData = CompileContext.GetInstanceDataView().GetPtr<FInstanceDataType>();
	check(InstanceData);

	EDataValidationResult Result = EDataValidationResult::Valid;

	// UOL 픽커에는 액터 클래스를 좁히는 엔진 확장점이 없으므로(필터를 걸 수 있는 자리가 엔진 Private 인 스톡 로케이터 에디터 안뿐이다) 컴파일에서 잡는다 — 드래그드롭으로 넣은 값도 같이 걸린다.
	// 해석되는데 스포너가 아닌 지정만 잡는다. 미해석(빈 로케이터·WP 언로드)은 타입을 알 수 없으므로 통과시킨다 — 에디터의 로드 상태에 따라 컴파일 결과가 갈리면 안 된다.
	for (const FUniversalObjectLocator& Locator : InstanceData->Spawners)
	{
		const UObject* Object = Locator.SyncFind();
		if (Object && !Object->IsA<AWxSpawner>())
		{
			CompileContext.AddValidationError(FText::Format(INVTEXT("Spawners: '{0}' 은(는) WxSpawner 가 아니다."), FText::FromString(UWxSpawnerLibrary::GetSpawnerLocatorDisplayName(Locator))));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

FText FWxStateTreeTask_WaitSpawnersKilled::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("스포너 처치 대기 ({0})"), UWxSpawnerLibrary::GetSpawnerLocatorsText(InstanceData->Spawners));
}
#endif
