// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_TriggerSpawners.h"

#include "GameFramework/Actor.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

FWxStateTreeTask_TriggerSpawners::FWxStateTreeTask_TriggerSpawners()
{
	// 진입 시 1회 트리거만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_TriggerSpawners::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 스폰을 재실행하지 않고 곧바로 완료한다(발동 순간에만 스폰).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 스폰은 서버 권위 사건이라 클라 진입은 노옵.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	for (const TSoftObjectPtr<AWxSpawner>& SoftSpawner : Instance.Spawners)
	{
		// 스트리밍 아웃된 스포너는 강제 로드하지 않고 스킵. 디자이너가 콘솔과 같은 영역에 배치되도록 보장해야 함.
		if (AWxSpawner* Spawner = SoftSpawner.Get())
		{
			Spawner->Respawn();
		}
		else
		{
			UE_LOG(LogWxWorld, Warning, TEXT("Trigger Spawners: TargetSpawner is null or not loaded."));
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_TriggerSpawners::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Trigger Spawners ({0})"), FText::AsNumber(InstanceData->Spawners.Num()));
}
#endif
