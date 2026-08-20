// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "System/WxSpawnerLibrary.h"
#include "WxWorldModule.h"

FWxStateTreeTask_TriggerSpawnersByLocator::FWxStateTreeTask_TriggerSpawnersByLocator()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_TriggerSpawnersByLocator::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
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

	int32 TriggeredCount = 0;
	for (const FUniversalObjectLocator& Locator : Instance.Spawners)
	{
		if (AWxSpawner* Spawner = Cast<AWxSpawner>(Locator.SyncFind(Owner)))
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

FText FWxStateTreeTask_TriggerSpawnersByLocator::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("스포너 발동 ({0})"), UWxSpawnerLibrary::GetSpawnerLocatorsText(InstanceData->Spawners));
}
#endif
