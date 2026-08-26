// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxSpawnerLocatorUtils.h"
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

	return FWxSpawnerLocatorUtils::ValidateSpawners(CompileContext, InstanceData->Spawners);
}

FText FWxStateTreeTask_TriggerSpawnersByLocator::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("스포너 발동 ({0})"), FWxLocatorUtils::GetDisplayNamesText(InstanceData->Spawners));
}
#endif
