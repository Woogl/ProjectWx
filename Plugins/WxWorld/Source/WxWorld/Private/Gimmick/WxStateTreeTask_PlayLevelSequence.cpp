// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_PlayLevelSequence.h"

#include "GameFramework/Actor.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_PlayLevelSequence::FWxStateTreeTask_PlayLevelSequence()
{
	// 재선택마다 재진입하면 재생 중인 시퀀스를 ExitState 가 정리하고 처음부터 다시 튼다. 컷신은 그 상태에 들어온 순간 한 번만 재생한다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 재생하지 않고 침묵 완료한다 — 컷신은 발동 순간에만 재생한다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 라이브 진입이지만 재생할 게 없으면, 상태가 갇히지 않게 곧장 완료한다(완료 전이가 다음 상태를 잇는다).
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

	// 플레이어 생성 실패면 정리하고 완료한다.
	if (!Instance.Player)
	{
		FinishSequencePlayback(Instance);
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.Player->Play();

	// 상태를 떠날 때까지 머문다. 재생 종료를 Tick 이 폴링해 완료시키고, 상태를 떠나면 ExitState 가 정리한다.
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 재생 중이면 계속 머문다.
	if (Instance.Player && Instance.Player->IsPlaying())
	{
		return EStateTreeRunStatus::Running;
	}

	// 재생이 끝났다. 시퀀스를 정리하고 완료해, 상태의 완료 전이가 다음 상태를 잇게 한다.
	FinishSequencePlayback(Instance);
	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_PlayLevelSequence::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 상태 이탈·중도 이탈·액터 파괴 시 시퀀스를 정지·정리한다(멱등).
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

	return FText::Format(INVTEXT("Play Level Sequence ({0})"),
		InstanceData->LevelSequence ? FText::FromString(InstanceData->LevelSequence->GetName()) : INVTEXT("none"));
}
#endif
