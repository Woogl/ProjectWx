// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxStateTreeTask_TriggerSpawners.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxSpawnerLocatorUtils.h"
#include "WxWorldModule.h"

FWxStateTreeTask_TriggerSpawners::FWxStateTreeTask_TriggerSpawners()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_TriggerSpawners::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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

	if (TriggeredCount == 0)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Trigger Spawners: 해석된 스포너가 없음(지정 %d개)."), Instance.Spawners.Num());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
EDataValidationResult FWxStateTreeTask_TriggerSpawners::Compile(UE::StateTree::ICompileNodeContext& CompileContext)
{
	const FInstanceDataType* InstanceData = CompileContext.GetInstanceDataView().GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FWxSpawnerLocatorUtils::ValidateSpawners(CompileContext, InstanceData->Spawners);
}

FText FWxStateTreeTask_TriggerSpawners::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("스포너 발동 ({0})"), FWxLocatorUtils::GetDisplayNamesText(InstanceData->Spawners));
}
#endif
