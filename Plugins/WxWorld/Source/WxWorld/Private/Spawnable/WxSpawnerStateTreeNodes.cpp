// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxSpawnerStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxWorldModule.h"

namespace
{
	/** 로케이터를 ST 오너 컨텍스트로 스포너로 해석한다. 빈 로케이터·미로드(스트리밍 아웃)·타입 불일치는 nullptr. */
	AWxSpawner* ResolveSpawner(const FUniversalObjectLocator& Locator, AActor* Owner)
	{
		return Cast<AWxSpawner>(Locator.SyncFind(Owner));
	}

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FString GetSpawnerDisplayName(const FUniversalObjectLocator& Locator)
	{
		if (Locator.IsEmpty())
		{
			return TEXT("none");
		}

		if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
		{
			return Actor->GetActorLabel();
		}

		// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
		const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
		const FActorLocatorFragment* Payload = nullptr;
		if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
		{
			const FString SubPath = Payload->Path.GetSubPathString();
			int32 DotIndex = INDEX_NONE;
			return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
		}

		return TEXT("unresolved");
	}

	/** 지정 목록의 표시 텍스트. 라벨 3개까지 나열하고 초과분은 +N 로 줄인다. 빈 배열은 none. */
	FText GetSpawnersText(const TArray<FUniversalObjectLocator>& Spawners)
	{
		if (Spawners.IsEmpty())
		{
			return INVTEXT("none");
		}

		constexpr int32 MaxNames = 3;
		TArray<FString> Names;
		for (int32 Index = 0; Index < Spawners.Num() && Index < MaxNames; ++Index)
		{
			Names.Add(GetSpawnerDisplayName(Spawners[Index]));
		}

		FString Joined = FString::Join(Names, TEXT(", "));
		if (Spawners.Num() > MaxNames)
		{
			Joined += FString::Printf(TEXT(" +%d"), Spawners.Num() - MaxNames);
		}
		return FText::FromString(Joined);
	}
#endif
}

// ── TriggerSpawnersByLocator ──────────────────────────────────────────────────

FWxStateTreeTask_TriggerSpawnersByLocator::FWxStateTreeTask_TriggerSpawnersByLocator()
{
	// 진입 시 1회 트리거만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_TriggerSpawnersByLocator::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 스폰을 재실행하지 않고 곧바로 완료한다(발동 순간에만 스폰).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 스폰은 서버 권위 사건이라 클라 진입은 노옵.
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 미해석(빈 로케이터·스트리밍 아웃)은 강제 로드 없이 스킵한다.
	int32 TriggeredCount = 0;
	for (const FUniversalObjectLocator& Locator : Instance.Spawners)
	{
		if (AWxSpawner* Spawner = ResolveSpawner(Locator, Owner))
		{
			Spawner->Respawn();
			++TriggeredCount;
		}
	}

	// 지정 누락이거나 전부 미해석이면 조립·배치 실수일 수 있다.
	if (TriggeredCount == 0)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Trigger Spawners By Locator: 해석된 스포너가 없음(지정 %d개)."), Instance.Spawners.Num());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
EDataValidationResult FWxStateTreeTask_TriggerSpawnersByLocator::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	const FInstanceDataType* InstanceData = CompileContext.GetInstanceDataView().GetPtr<FInstanceDataType>();
	check(InstanceData);

	EDataValidationResult Result = EDataValidationResult::Valid;

	// 해석되는데 스포너가 아닌 지정만 잡는다. 미해석(빈 로케이터·WP 언로드)은 타입을 알 수 없으므로 통과시킨다 — 에디터의 로드 상태에 따라 컴파일 결과가 갈리면 안 된다.
	for (const FUniversalObjectLocator& Locator : InstanceData->Spawners)
	{
		const UObject* Object = Locator.SyncFind();
		if (Object && !Object->IsA<AWxSpawner>())
		{
			CompileContext.AddValidationError(FText::Format(INVTEXT("Spawners: '{0}' 은(는) WxSpawner 가 아니다."), FText::FromString(GetSpawnerDisplayName(Locator))));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

FText FWxStateTreeTask_TriggerSpawnersByLocator::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Trigger Spawners By Locator ({0})"), GetSpawnersText(InstanceData->Spawners));
}
#endif

// ── WaitSpawnersKilled ────────────────────────────────────────────────────────

FWxStateTreeTask_WaitSpawnersKilled::FWxStateTreeTask_WaitSpawnersKilled()
{
	// 처치 대기 중 같은 상태가 재선택되어도 진행을 끊을 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitSpawnersKilled::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

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

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitSpawnersKilled::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 지정이 없으면 판정 대상이 없어 완료할 수 없다(진입 시 이미 경고를 남겼다).
	if (Instance.Spawners.IsEmpty())
	{
		return EStateTreeRunStatus::Running;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Running;
	}

	// 전원이 해석(로드)되고 처치여야 통과한다. 미해석은 판정 불가라 강제 로드 없이 대기한다.
	for (const FUniversalObjectLocator& Locator : Instance.Spawners)
	{
		const AWxSpawner* Spawner = ResolveSpawner(Locator, Owner);
		if (!Spawner || !Spawner->IsKilled())
		{
			return EStateTreeRunStatus::Running;
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
EDataValidationResult FWxStateTreeTask_WaitSpawnersKilled::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	const FInstanceDataType* InstanceData = CompileContext.GetInstanceDataView().GetPtr<FInstanceDataType>();
	check(InstanceData);

	EDataValidationResult Result = EDataValidationResult::Valid;

	// 해석되는데 스포너가 아닌 지정만 잡는다. 미해석(빈 로케이터·WP 언로드)은 타입을 알 수 없으므로 통과시킨다 — 에디터의 로드 상태에 따라 컴파일 결과가 갈리면 안 된다.
	for (const FUniversalObjectLocator& Locator : InstanceData->Spawners)
	{
		const UObject* Object = Locator.SyncFind();
		if (Object && !Object->IsA<AWxSpawner>())
		{
			CompileContext.AddValidationError(FText::Format(INVTEXT("Spawners: '{0}' 은(는) WxSpawner 가 아니다."), FText::FromString(GetSpawnerDisplayName(Locator))));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

FText FWxStateTreeTask_WaitSpawnersKilled::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wait Spawners Killed ({0})"), GetSpawnersText(InstanceData->Spawners));
}
#endif
