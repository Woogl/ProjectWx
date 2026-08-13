// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_PlaySound.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_PlaySound::FWxStateTreeTask_PlaySound()
{
	// 진입 시 1회 재생만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlaySound::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 기본적으로 재생하지 않고 곧바로 완료한다.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry && !Instance.bPlayOnRestore)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, Instance.Sound, Owner->GetActorLocation());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlaySound::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("사운드 재생 ({0})"),
		InstanceData->Sound ? FText::FromString(InstanceData->Sound->GetName()) : INVTEXT("none"));
}
#endif
