// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "WxCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Time/WxTimeDilationComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	UWxCombatLibrary::RemoveEffect(AbilitySystemComponent.Get(), UWxEffect_Invincible::StaticClass());

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
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}

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
	ACharacter* AvatarCharacter = Cast<ACharacter>(AvatarActor);
	if (AvatarCharacter)
	{
		AvatarCharacter->StopAnimMontage();
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, PlaybackSettings, NewSequenceActor);
	SequenceActor = NewSequenceActor;

	if (!SequencePlayer)
	{
		UWxTimeDilationComponent::ClearGlobalTimeDilationAuthoritative(this);
		CleanupSequenceActor();

		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}

		EndTask();
		return;
	}

	if (AvatarActor && SequenceActor)
	{
		SequenceActor->bOverrideInstanceData = true;
		if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData))
		{
			// 시퀀스는 레퍼런스 스켈레탈 메시를 원점에 두고 저작하므로, 아바타 쪽 대응물도 액터가 아니라 메시가 놓인 자리다.
			InstanceData->TransformOrigin = AvatarCharacter ? AvatarCharacter->GetMesh()->GetComponentTransform() : AvatarActor->GetActorTransform();
		}

		static const FName PlayerBindingTag = TEXT("Player");
		TArray<AActor*> Actors;
		Actors.Add(AvatarActor);
		// 에셋의 자체 바인딩을 함께 허용하면 저작용 레퍼런스 액터가 게임에서도 스폰된다.
		SequenceActor->SetBindingByTag(PlayerBindingTag, Actors, false);
	}

	if (GlobalTimeDilation > 0.f && GlobalTimeDilation != 1.f)
	{
		SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation);
	}

	// 무적의 수명은 이 태스크가 쥔다 — 시퀀스가 끝나든 어빌리티가 캔슬되든 OnDestroy에서 걷힌다.
	UWxCombatLibrary::ApplyEffect(AbilitySystemComponent.Get(), UWxEffect_Invincible::StaticClass(), Ability);

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
