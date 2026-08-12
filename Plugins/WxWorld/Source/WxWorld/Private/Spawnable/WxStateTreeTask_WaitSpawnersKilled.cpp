// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_WaitSpawnersKilled.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "System/WxSpawnerLibrary.h"
#include "WxWorldModule.h"

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
		const AWxSpawner* Spawner = Cast<AWxSpawner>(Locator.SyncFind(Owner));
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

	return FText::Format(INVTEXT("Wait Spawners Killed ({0})"), UWxSpawnerLibrary::GetSpawnerLocatorsText(InstanceData->Spawners));
}
#endif
