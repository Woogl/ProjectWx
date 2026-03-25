// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlayCutscene.h"
#include "AbilitySystemComponent.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Kismet/GameplayStatics.h"
#include "WxGameplayTags.h"

UWxAbilityTask_PlayCutscene* UWxAbilityTask_PlayCutscene::CreateTask(UGameplayAbility* OwningAbility, ULevelSequence* InLevelSequence)
{
	UWxAbilityTask_PlayCutscene* Task = NewAbilityTask<UWxAbilityTask_PlayCutscene>(OwningAbility);
	Task->LevelSequence = InLevelSequence;
	return Task;
}

void UWxAbilityTask_PlayCutscene::Activate()
{
	Super::Activate();

	UWorld* World = GetWorld();
	if (!World || !LevelSequence)
	{
		OnCancelled.Broadcast();
		EndTask();
		return;
	}

	// 무적 태그 부여
	AddInvincibleTag();

	// Time Dilation 설정
	constexpr float CutsceneTimeDilation = 0.001f;
	OriginalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
	UGameplayStatics::SetGlobalTimeDilation(World, CutsceneTimeDilation);
	bTimeDilationModified = true;

	// Level Sequence Actor 스폰 및 재생
	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, PlaybackSettings, NewSequenceActor);
	SequenceActor = NewSequenceActor;

	if (!SequencePlayer)
	{
		RestoreTimeDilation();
		CleanupSequenceActor();
		OnCancelled.Broadcast();
		EndTask();
		return;
	}

	// PlayRate를 Time Dilation의 역수로 보정하여 시퀀스만 정상 속도로 재생
	SequencePlayer->SetPlayRate(1.f / CutsceneTimeDilation);
	SequencePlayer->OnFinished.AddDynamic(this, &UWxAbilityTask_PlayCutscene::HandleSequenceFinished);
	SequencePlayer->Play();
}

void UWxAbilityTask_PlayCutscene::OnDestroy(bool bInOwnerFinished)
{
	RemoveInvincibleTag();
	RestoreTimeDilation();
	CleanupSequenceActor();

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_PlayCutscene::HandleSequenceFinished()
{
	RestoreTimeDilation();
	CleanupSequenceActor();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast();
	}

	EndTask();
}

void UWxAbilityTask_PlayCutscene::RestoreTimeDilation()
{
	if (bTimeDilationModified)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UGameplayStatics::SetGlobalTimeDilation(World, OriginalTimeDilation);
		}
		bTimeDilationModified = false;
	}
}

void UWxAbilityTask_PlayCutscene::AddInvincibleTag()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::ANS_Invincible);
		bInvincibleTagAdded = true;
	}
}

void UWxAbilityTask_PlayCutscene::RemoveInvincibleTag()
{
	if (bInvincibleTagAdded)
	{
		UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_Invincible);
		}
		bInvincibleTagAdded = false;
	}
}

void UWxAbilityTask_PlayCutscene::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}
