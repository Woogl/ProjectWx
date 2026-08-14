// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "AbilitySystemComponent.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Time/WxTimeDilationComponent.h"
#include "GameFramework/Character.h"

UWxAbilityTask_PlaySkillCutscene* UWxAbilityTask_PlaySkillCutscene::CreateTask(UGameplayAbility* OwningAbility, ULevelSequence* InLevelSequence, float InGlobalTimeDilation)
{
	UWxAbilityTask_PlaySkillCutscene* Task = NewAbilityTask<UWxAbilityTask_PlaySkillCutscene>(OwningAbility);
	Task->LevelSequence = InLevelSequence;
	Task->GlobalTimeDilation = InGlobalTimeDilation;
	return Task;
}

void UWxAbilityTask_PlaySkillCutscene::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->RemoveActiveGameplayEffect(InvincibleHandle);
	}

	UWxTimeDilationComponent::ClearGlobalTimeDilationAuthoritative(this);
	CleanupSequenceActor();

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_PlaySkillCutscene::Activate()
{
	Super::Activate();

	UWorld* World = GetWorld();
	if (!World || !LevelSequence)
	{
		OnCancelled.Broadcast();
		EndTask();
		return;
	}

	// 0·음수 TimeDilation은 시간이 아예 멈춰 복원 경로가 돌지 않는다.
	if (GlobalTimeDilation <= 0.f)
	{
		GlobalTimeDilation = 0.001f;
	}

	UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(this, GlobalTimeDilation);

	AActor* AvatarActor = GetAvatarActor();
	if (AvatarActor)
	{
		if (ACharacter* AvatarCharacter = Cast<ACharacter>(AvatarActor))
		{
			AvatarCharacter->StopAnimMontage();
		}
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, PlaybackSettings, NewSequenceActor);
	SequenceActor = NewSequenceActor;

	if (!SequencePlayer)
	{
		UWxTimeDilationComponent::ClearGlobalTimeDilationAuthoritative(this);
		CleanupSequenceActor();
		OnCancelled.Broadcast();
		EndTask();
		return;
	}

	if (AvatarActor && SequenceActor)
	{
		SequenceActor->bOverrideInstanceData = true;
		if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData))
		{
			InstanceData->TransformOrigin = AvatarActor->GetActorTransform();
		}

		static const FName PlayerBindingTag = TEXT("Player");
		TArray<AActor*> Actors;
		Actors.Add(AvatarActor);
		SequenceActor->SetBindingByTag(PlayerBindingTag, Actors, true);
	}

	if (GlobalTimeDilation > 0.f && GlobalTimeDilation != 1.f)
	{
		SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation);
	}

	// 시퀀스는 딜레이션을 상쇄한 배속으로 돌아 실시간 기준 GetDuration()만큼 걸리지만,
	// GE 지속시간은 딜레이션이 걸린 월드 시간으로 세므로 그만큼 줄여 준다.
	InvincibleHandle = UWxEffect_Invincible::ApplyTo(
		AbilitySystemComponent.Get(),
		SequencePlayer->GetDuration().AsSeconds() * GlobalTimeDilation,
		Ability);

	SequencePlayer->OnFinished.AddDynamic(this, &UWxAbilityTask_PlaySkillCutscene::HandleSequenceFinished);
	SequencePlayer->Play();
}

void UWxAbilityTask_PlaySkillCutscene::HandleSequenceFinished()
{
	UWxTimeDilationComponent::ClearGlobalTimeDilationAuthoritative(this);
	CleanupSequenceActor();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast();
	}

	EndTask();
}

void UWxAbilityTask_PlaySkillCutscene::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}
