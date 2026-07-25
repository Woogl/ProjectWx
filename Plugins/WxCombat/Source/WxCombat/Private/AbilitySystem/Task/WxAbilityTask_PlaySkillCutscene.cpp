// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "AbilitySystemComponent.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Kismet/GameplayStatics.h"
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
	RestoreTimeDilation();
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

	// 무적 태그 부여
	AddInvincibleTag();

	// TimeDilation으로 0, 음수 사용 방지
	if (GlobalTimeDilation <= 0.f)
	{
		GlobalTimeDilation = 0.001f;
	}
	
	// Time Dilation 설정
	OriginalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
	UGameplayStatics::SetGlobalTimeDilation(World, GlobalTimeDilation);
	bTimeDilationActive = true;
	
	// 캐릭터가 재생 중이던 애님 몽타주 정지
	AActor* AvatarActor = GetAvatarActor();
	if (AvatarActor)
	{
		if (ACharacter* AvatarCharacter = Cast<ACharacter>(AvatarActor))
		{
			AvatarCharacter->StopAnimMontage();
		}
	}

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

	// AvatarActor 기준으로 TransformOrigin 및 액터 리바인딩 설정
	if (AvatarActor && SequenceActor)
	{
		// TransformOrigin: AvatarActor의 Transform을 시퀀스 원점으로 사용
		SequenceActor->bOverrideInstanceData = true;
		if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData))
		{
			InstanceData->TransformOrigin = AvatarActor->GetActorTransform();
		}

		// 액터 리바인딩: 시퀀스에서 Binding Tag가 "Player"인 바인딩을 AvatarActor로 교체
		static const FName PlayerBindingTag = TEXT("Player");
		TArray<AActor*> Actors;
		Actors.Add(AvatarActor);
		SequenceActor->SetBindingByTag(PlayerBindingTag, Actors, true);
	}

	// PlayRate를 Time Dilation의 역수로 보정하여 시퀀스만 정상 속도로 재생
	if (GlobalTimeDilation > 0.f && GlobalTimeDilation != 1.f)
	{
		SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation);
	}

	SequencePlayer->OnFinished.AddDynamic(this, &UWxAbilityTask_PlaySkillCutscene::HandleSequenceFinished);
	SequencePlayer->Play();
}

void UWxAbilityTask_PlaySkillCutscene::HandleSequenceFinished()
{
	RestoreTimeDilation();
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
	// 루즈 태그는 레퍼런스 카운트라, 부여하지 않았는데 제거하면 남이 열어 둔 무적 창(ANS_Invincible 등)을 대신 걷어낸다.
	// Activate 가 조기 종료해 AddInvincibleTag 를 지나치지 못한 채 OnDestroy 로 흐르는 경로가 있다.
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

void UWxAbilityTask_PlaySkillCutscene::RestoreTimeDilation()
{
	if (!bTimeDilationActive)
	{
		return;
	}
	bTimeDilationActive = false;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, OriginalTimeDilation);
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
