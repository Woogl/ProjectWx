// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_RespawnSpawners.h"

#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "System/WxSpawnerLibrary.h"

FWxStateTreeTask_RespawnSpawners::FWxStateTreeTask_RespawnSpawners()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_RespawnSpawners::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 호출하지 않는다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 스폰은 서버 권위 사건이다(스포너 내부에서도 한 번 더 가린다).
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (Owner && Owner->HasAuthority())
	{
		UWxSpawnerLibrary::TryRespawnAll(Owner);
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_RespawnSpawners::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return INVTEXT("스포너 리스폰");
}
#endif
