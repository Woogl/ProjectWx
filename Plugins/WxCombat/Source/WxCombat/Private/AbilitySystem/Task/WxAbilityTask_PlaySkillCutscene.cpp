// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "AbilitySystemComponent.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Time/WxTimeDilationComponent.h"
#include "WxGameplayTags.h"
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
	RemoveInvincibleTag();
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

	AddInvincibleTag();

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

		// 시퀀스에서 Binding Tag가 "Player"인 바인딩을 AvatarActor로 교체한다.
		static const FName PlayerBindingTag = TEXT("Player");
		TArray<AActor*> Actors;
		Actors.Add(AvatarActor);
		SequenceActor->SetBindingByTag(PlayerBindingTag, Actors, true);
	}

	// Time Dilation의 역수로 보정해 시퀀스만 정상 속도로 재생한다.
	if (GlobalTimeDilation > 0.f && GlobalTimeDilation != 1.f)
	{
		SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation);
	}

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

void UWxAbilityTask_PlaySkillCutscene::AddInvincibleTag()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Invincible);
		bInvincibleTagAdded = true;
	}
}

void UWxAbilityTask_PlaySkillCutscene::RemoveInvincibleTag()
{
	// 루스 태그는 레퍼런스 카운트라, 부여하지 않았는데 제거하면 남이 열어 둔 무적 창을 대신 걷어낸다.
	// Activate가 조기 종료해 AddInvincibleTag를 지나치지 못한 채 OnDestroy로 흐르는 경로가 있다.
	if (!bInvincibleTagAdded)
	{
		return;
	}
	bInvincibleTagAdded = false;

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Invincible);
	}
}

void UWxAbilityTask_PlaySkillCutscene::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}
