// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_PlayLevelSequence.h"

#include "GameFramework/Actor.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_PlayLevelSequence::FWxStateTreeTask_PlayLevelSequence()
{
	// 재선택마다 재진입하면 재생 중인 시퀀스를 ExitState 가 정리하고 처음부터 다시 튼다.
	// 컷신은 그 상태에 들어온 순간 한 번만 재생한다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!Instance.LevelSequence || !World)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	Instance.Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Instance.LevelSequence, PlaybackSettings, NewSequenceActor);
	Instance.SequenceActor = NewSequenceActor;

	if (!Instance.Player)
	{
		FinishSequencePlayback(Instance);
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.Player->Play();

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.Player && Instance.Player->IsPlaying())
	{
		return EStateTreeRunStatus::Running;
	}

	FinishSequencePlayback(Instance);
	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_PlayLevelSequence::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	FinishSequencePlayback(Instance);
}

void FWxStateTreeTask_PlayLevelSequence::FinishSequencePlayback(FInstanceDataType& Instance) const
{
	if (Instance.Player)
	{
		Instance.Player->Stop();
	}

	if (Instance.SequenceActor)
	{
		Instance.SequenceActor->Destroy();
	}

	Instance.SequenceActor = nullptr;
	Instance.Player = nullptr;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayLevelSequence::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("레벨 시퀀스 재생 ({0})"),
		InstanceData->LevelSequence ? FText::FromString(InstanceData->LevelSequence->GetName()) : INVTEXT("none"));
}
#endif
