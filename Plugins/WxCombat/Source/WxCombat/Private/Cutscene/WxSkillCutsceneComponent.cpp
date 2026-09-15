// Copyright Woogle. All Rights Reserved.

#include "Cutscene/WxSkillCutsceneComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneTimeController.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Sections/MovieSceneSkeletalAnimationSection.h"
#include "Net/UnrealNetwork.h"
#include "WxCombatModule.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

namespace
{
	ULevelSequence* CreatePlayerCutsceneSequence(ULevelSequence* Source, UObject* Outer)
	{
		// 에셋은 PIE 월드끼리 공유한다. 원본 섹션을 수정하면 다른 플레이어와 에디터까지 영향을 받는다.
		ULevelSequence* PlaybackSequence = DuplicateObject<ULevelSequence>(Source, Outer,
			MakeUniqueObjectName(Outer, Source->GetClass(), Source->GetFName()));
		UMovieScene* MovieScene = PlaybackSequence->GetMovieScene();
		TSet<FGuid> PlayerBindings;
		for (const FMovieSceneObjectBindingID& BindingId : PlaybackSequence->FindBindingsByTag(TEXT("Player")))
		{
			PlayerBindings.Add(BindingId.GetGuid());
		}
		for (const FMovieSceneBinding& Binding : static_cast<const UMovieScene*>(MovieScene)->GetBindings())
		{
			FGuid OwnerId = Binding.GetObjectGuid();
			while (OwnerId.IsValid() && !PlayerBindings.Contains(OwnerId))
			{
				const FMovieScenePossessable* Possessable = MovieScene->FindPossessable(OwnerId);
				OwnerId = Possessable ? Possessable->GetParent() : FGuid();
			}
			if (!PlayerBindings.Contains(OwnerId))
			{
				continue;
			}
			for (UMovieSceneTrack* Track : Binding.GetTracks())
			{
				if (UMovieSceneSkeletalAnimationTrack* AnimationTrack = Cast<UMovieSceneSkeletalAnimationTrack>(Track))
				{
					for (UMovieSceneSection* Section : AnimationTrack->GetAllSections())
					{
						if (UMovieSceneSkeletalAnimationSection* AnimationSection = Cast<UMovieSceneSkeletalAnimationSection>(Section))
						{
							// 대기/이동 AnimBP의 슬롯 경로를 거치지 않고 시퀀서가 시전자 포즈를 소유한다.
							AnimationSection->Params.bForceCustomMode = true;
						}
					}
				}
			}
		}
		return PlaybackSequence;
	}

	/** 플랫폼 시각에서 직접 프레임을 구하므로 월드 배율이나 시퀀스의 Clock Source에 의존하지 않는다. */
	class FWxSkillCutsceneClock : public FMovieSceneTimeController
	{
	public:
		explicit FWxSkillCutsceneClock(UWxSkillCutsceneComponent* InCoordinator, double InStartSeconds);

	protected:
		virtual FFrameTime OnRequestCurrentTime(const FQualifiedFrameTime& InCurrentTime, float InPlayRate) override;

	private:
		TWeakObjectPtr<UWxSkillCutsceneComponent> Coordinator;
		double StartSeconds;
	};

	FWxSkillCutsceneClock::FWxSkillCutsceneClock(UWxSkillCutsceneComponent* InCoordinator, double InStartSeconds)
		: Coordinator(InCoordinator), StartSeconds(InStartSeconds)
	{
	}

	FFrameTime FWxSkillCutsceneClock::OnRequestCurrentTime(const FQualifiedFrameTime& InCurrentTime, float InPlayRate)
	{
		if (const UWxSkillCutsceneComponent* Owner = Coordinator.Get())
		{
			return InCurrentTime.Rate.AsFrameTime(StartSeconds + Owner->GetPlaybackSeconds());
		}
		return InCurrentTime.Time;
	}
}

UWxSkillCutsceneComponent::UWxSkillCutsceneComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

UWxSkillCutsceneComponent* UWxSkillCutsceneComponent::Get(const UWorld* World)
{
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<UWxSkillCutsceneComponent>() : nullptr;
}

bool UWxSkillCutsceneComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

bool UWxSkillCutsceneComponent::IsBusy() const
{
	return Reservation.IsValid() || State.Phase != EWxSkillCutscenePhase::Idle;
}

bool UWxSkillCutsceneComponent::IsForAvatar(const AActor* Avatar) const
{
	return State.Phase != EWxSkillCutscenePhase::Idle && State.Avatar == Avatar;
}

bool UWxSkillCutsceneComponent::Reserve(UGameplayAbility* Requester)
{
	if (!HasAuthority() || !Requester || IsBusy())
	{
		return false;
	}
	Reservation = Requester;
	++State.Id;
	return true;
}

bool UWxSkillCutsceneComponent::Start(UGameplayAbility* Requester, ULevelSequence* Sequence, float Dilation)
{
	if (!HasAuthority() || !Requester || Reservation.Get() != Requester || State.Phase != EWxSkillCutscenePhase::Idle)
	{
		return false;
	}

	UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;
	AActor* Avatar = Requester->GetAvatarActorFromActorInfo();
	if (!MovieScene || !IsValid(Avatar) || !FMath::IsFinite(Dilation))
	{
		return false;
	}
	const TRange<FFrameNumber> Range = MovieScene->GetPlaybackRange();
	if (!Range.HasLowerBound() || !Range.HasUpperBound())
	{
		return false;
	}
	const double Duration = MovieScene->GetTickResolution().AsSeconds(Range.Size<FFrameNumber>());
	if (Duration <= 0.0 || !FMath::IsFinite(Duration))
	{
		return false;
	}

	State.Sequence = Sequence;
	State.Avatar = Avatar;
	State.Origin = Avatar->GetActorTransform();
	if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			State.Origin = Mesh->GetComponentTransform();
		}
	}
	State.Duration = Duration;
	ServerPlaybackStartTime = 0.0;
	State.bCancelled = false;
	State.Phase = EWxSkillCutscenePhase::Playing;
	RequestedDilation = FMath::Max(Dilation, 0.001f);
	PreparationDeadline = FPlatformTime::Seconds() + 20.0;
	bAvatarWasAlwaysRelevant = Avatar->bAlwaysRelevant;
	bAvatarRelevancyChanged = true;
	Avatar->bAlwaysRelevant = true;
	Avatar->FlushNetDormancy();
	Avatar->ForceNetUpdate();

	if (UAbilitySystemComponent* ASC = Requester->GetAbilitySystemComponentFromActorInfo())
	{
		InvincibleASC = ASC;
		InvincibleHandle = ASC->ApplyGameplayEffectToSelf(GetDefault<UWxEffect_Invincible>(), Requester->GetAbilityLevel(), ASC->MakeEffectContext());
	}

	OnRep_State();
	GetOwner()->ForceNetUpdate();
	return true;
}

void UWxSkillCutsceneComponent::Cancel(UGameplayAbility* Requester)
{
	if (HasAuthority() && Requester && Reservation.Get() == Requester)
	{
		Finish(true);
	}
}

uint32 UWxSkillCutsceneComponent::GetSessionId() const
{
	return State.Id;
}

double UWxSkillCutsceneComponent::GetPlaybackSeconds() const
{
	return bLocalPlaying ? FMath::Clamp(FPlatformTime::Seconds() - LocalPlaybackStartTime, 0.0, LocalState.Duration) : 0.0;
}

void UWxSkillCutsceneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWxSkillCutsceneComponent, State);
}

void UWxSkillCutsceneComponent::OnRep_State()
{
	if (State.Id != ObservedSessionId)
	{
		ObservedSessionId = State.Id;
		if (State.Phase == EWxSkillCutscenePhase::Playing && GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			PendingLocalSessions.Add(State);
		}
	}
	// 참조가 늦게 해석되어도 같은 시전자를 기다리며 로컬 폰으로 대체하지 않는다.
	if (bLocalActive && LocalState.Id == State.Id && State.Avatar)
	{
		LocalState.Avatar = State.Avatar;
	}
	for (int32 Index = PendingLocalSessions.Num() - 1; Index >= 0; --Index)
	{
		if (PendingLocalSessions[Index].Id == State.Id)
		{
			PendingLocalSessions[Index].Avatar = State.Avatar;
			if (State.bCancelled)
			{
				PendingLocalSessions.RemoveAt(Index);
			}
		}
	}
	if (State.Phase == EWxSkillCutscenePhase::Idle && State.bCancelled && bLocalActive && LocalState.Id == State.Id)
	{
		CleanupLocalPlayer();
	}
	BeginNextLocalSession();
	if (!HasAuthority() && State.Phase == EWxSkillCutscenePhase::Idle && State.Id != 0 && LastReceivedEndId != State.Id)
	{
		LastReceivedEndId = State.Id;
		PendingClientCompletions.Add(State);
	}
	NotifyReadyClientCompletions();
}

void UWxSkillCutsceneComponent::NotifyReadyClientCompletions()
{
	for (int32 Index = 0; Index < PendingClientCompletions.Num();)
	{
		const FWxSkillCutsceneState Completion = PendingClientCompletions[Index];
		bool bWaitingForLocal = bLocalActive && LocalState.Id == Completion.Id;
		for (const FWxSkillCutsceneState& Pending : PendingLocalSessions)
		{
			bWaitingForLocal |= Pending.Id == Completion.Id;
		}
		if (!Completion.bCancelled && bWaitingForLocal)
		{
			++Index;
			continue;
		}
		// 완료 콜백이 다음 어빌리티를 시작할 수 있으므로 먼저 대기 목록에서 제거한다.
		PendingClientCompletions.RemoveAt(Index);
		OnCutsceneEnded.Broadcast(Completion.Avatar, Completion.Id, Completion.bCancelled);
		Index = 0;
	}
}

void UWxSkillCutsceneComponent::BeginNextLocalSession()
{
	if (bLocalActive || PendingLocalSessions.IsEmpty())
	{
		return;
	}
	LocalState = PendingLocalSessions[0];
	PendingLocalSessions.RemoveAt(0);
	bLocalActive = true;
	PreparationDeadline = FPlatformTime::Seconds() + 20.0;
	if (UWorldPartitionSubsystem* Partition = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>())
	{
		Partition->RegisterStreamingSourceProvider(this);
		bStreamingSourceRegistered = true;
		StreamingSourceFrame = GFrameCounter;
	}
}

void UWxSkillCutsceneComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	if (HasAuthority() && State.Phase != EWxSkillCutscenePhase::Idle)
	{
		if (!Reservation.IsValid() || !IsValid(State.Avatar))
		{
			Finish(true);
			return;
		}
		if (GetWorld()->GetNetMode() == NM_DedicatedServer && ServerPlaybackStartTime == 0.0)
		{
			PreviousDilation = UGameplayStatics::GetGlobalTimeDilation(this);
			UGameplayStatics::SetGlobalTimeDilation(this, RequestedDilation);
			bDilationApplied = true;
			ServerPlaybackStartTime = FPlatformTime::Seconds();
		}
		if (GetWorld()->GetNetMode() == NM_DedicatedServer && ServerPlaybackStartTime > 0.0 && FPlatformTime::Seconds() - ServerPlaybackStartTime >= State.Duration)
		{
			Finish(false);
			return;
		}
	}
	BeginNextLocalSession();
	if (!bLocalActive)
	{
		return;
	}
	if (!bLocalPlaying && FPlatformTime::Seconds() >= PreparationDeadline)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("컷신 %u: 로컬 준비 20초 시간 초과."), LocalState.Id);
		if (HasAuthority())
		{
			Finish(true);
		}
		else
		{
			CleanupLocalPlayer();
		}
		return;
	}
	PrepareLocalPlayer();
}

void UWxSkillCutsceneComponent::PrepareLocalPlayer()
{
	if (!bLocalActive || bLocalPlaying)
	{
		return;
	}
	if (!LocalState.Sequence.Get() && !SequenceLoad)
	{
		SequenceLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(LocalState.Sequence.ToSoftObjectPath());
	}
	if (SequenceLoad && !SequenceLoad->HasLoadCompleted())
	{
		return;
	}
	bool bFailed = LocalState.Sequence.Get() == nullptr;
	if (!bFailed && (!IsValid(LocalState.Avatar) || !IsSceneReady()))
	{
		return;
	}
	if (!bFailed && !LocalSequenceActor)
	{
		FMovieSceneSequencePlaybackSettings Settings;
		Settings.bDisableMovementInput = true;
		Settings.bDisableLookAtInput = true;
		Settings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceRestoreState;
		ALevelSequenceActor* NewActor = nullptr;
		ULevelSequence* PlaybackSequence = CreatePlayerCutsceneSequence(LocalState.Sequence.Get(), this);
		ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), PlaybackSequence, Settings, NewActor);
		LocalSequenceActor = NewActor;
		bFailed = !Player || !NewActor || NewActor->FindNamedBindings(TEXT("Player")).IsEmpty();
		if (!bFailed)
		{
			NewActor->bOverrideInstanceData = true;
			UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(NewActor->DefaultInstanceData);
			bFailed = InstanceData == nullptr;
			if (InstanceData)
			{
				InstanceData->TransformOrigin = LocalState.Origin;
				TArray<AActor*> Actors;
				Actors.Add(LocalState.Avatar);
				NewActor->SetBindingByTag(TEXT("Player"), Actors, false);
				Player->SetTimeController(MakeShared<FWxSkillCutsceneClock>(this, Player->GetStartTime().AsSeconds()));
			}
		}
	}
	if (bFailed)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("컷신 %u: 로컬 시퀀스 또는 Player 바인딩 준비 실패."), LocalState.Id);
		if (HasAuthority())
		{
			Finish(true);
		}
		else
		{
			CleanupLocalPlayer();
		}
		return;
	}
	StartLocalPlayer();
}

void UWxSkillCutsceneComponent::StartLocalPlayer()
{
	if (bLocalPlaying || !LocalSequenceActor)
	{
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(LocalState.Avatar))
	{
		Character->StopAnimMontage();
	}
	EnableLocalMeshPoseTick();
	LocalPlaybackStartTime = FPlatformTime::Seconds();
	bLocalPlaying = true;
	if (HasAuthority())
	{
		PreviousDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		UGameplayStatics::SetGlobalTimeDilation(this, RequestedDilation);
		bDilationApplied = true;
		ServerPlaybackStartTime = LocalPlaybackStartTime;
	}
	// 각 머신이 시퀀스 범위의 시작부터 재생한다. 서버 시간으로 건너뛰지 않는다.
	LocalSequenceActor->GetSequencePlayer()->OnFinished.AddDynamic(this, &UWxSkillCutsceneComponent::HandleLocalSequenceFinished);
	LocalSequenceActor->GetSequencePlayer()->Play();
}

void UWxSkillCutsceneComponent::HandleLocalSequenceFinished()
{
	if (HasAuthority())
	{
		Finish(false);
	}
	else
	{
		CleanupLocalPlayer();
	}
}

void UWxSkillCutsceneComponent::EnableLocalMeshPoseTick()
{
	const ACharacter* Character = Cast<ACharacter>(LocalState.Avatar);
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || LocalPoseMesh.IsValid())
	{
		return;
	}

	LocalPoseMesh = Mesh;
	bPreviousOnlyAllowAutonomousTickPose = Mesh->bOnlyAllowAutonomousTickPose;
	// 서버의 원격 시전자는 평소 이동 패킷이 포즈를 갱신한다. 컷신은 일반 메시 틱에서도 평가되어야 한다.
	Mesh->bOnlyAllowAutonomousTickPose = false;
}

void UWxSkillCutsceneComponent::CleanupLocalPlayer()
{
	if (LocalSequenceActor)
	{
		if (ULevelSequencePlayer* Player = LocalSequenceActor->GetSequencePlayer())
		{
			Player->OnFinished.RemoveDynamic(this, &UWxSkillCutsceneComponent::HandleLocalSequenceFinished);
			Player->Stop();
			Player->RestoreState();
		}
		LocalSequenceActor->Destroy();
		LocalSequenceActor = nullptr;
	}
	if (USkeletalMeshComponent* Mesh = LocalPoseMesh.Get())
	{
		Mesh->bOnlyAllowAutonomousTickPose = bPreviousOnlyAllowAutonomousTickPose;
	}
	LocalPoseMesh.Reset();
	SequenceLoad.Reset();
	if (bStreamingSourceRegistered)
	{
		if (UWorldPartitionSubsystem* Partition = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>())
		{
			Partition->UnregisterStreamingSourceProvider(this);
		}
		bStreamingSourceRegistered = false;
	}
	bLocalActive = false;
	bLocalPlaying = false;
	LocalPlaybackStartTime = 0.0;
	NotifyReadyClientCompletions();
}

void UWxSkillCutsceneComponent::RestoreServerState()
{
	if (bDilationApplied)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, PreviousDilation);
		bDilationApplied = false;
	}
	if (UAbilitySystemComponent* ASC = InvincibleASC.Get())
	{
		ASC->RemoveActiveGameplayEffect(InvincibleHandle);
	}
	InvincibleHandle.Invalidate();
	InvincibleASC.Reset();
	if (bAvatarRelevancyChanged && IsValid(State.Avatar))
	{
		State.Avatar->bAlwaysRelevant = bAvatarWasAlwaysRelevant;
		State.Avatar->ForceNetUpdate();
	}
	bAvatarRelevancyChanged = false;
}

void UWxSkillCutsceneComponent::Finish(bool bCancelled)
{
	const uint32 FinishedId = State.Id;
	RestoreServerState();
	CleanupLocalPlayer();
	Reservation.Reset();
	PendingLocalSessions.Reset();
	State.Phase = EWxSkillCutscenePhase::Idle;
	State.bCancelled = bCancelled;
	// 마지막 상태에도 장면 정보를 남겨 늦게 해석되는 참조를 보존한다.
	GetOwner()->ForceNetUpdate();
	OnCutsceneEnded.Broadcast(State.Avatar, FinishedId, bCancelled);
}

void UWxSkillCutsceneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PendingClientCompletions.Reset();
	if (HasAuthority() && IsBusy())
	{
		Finish(true);
	}
	else
	{
		CleanupLocalPlayer();
	}
	PendingLocalSessions.Reset();
	Super::EndPlay(EndPlayReason);
}

bool UWxSkillCutsceneComponent::GetStreamingSource(FWorldPartitionStreamingSource& OutSource) const
{
	if (!bLocalActive)
	{
		return false;
	}
	OutSource = FWorldPartitionStreamingSource(GetFName(), LocalState.Origin.GetLocation(), LocalState.Origin.Rotator(),
		EStreamingSourceTargetState::Activated, false, EStreamingSourcePriority::High, false);
	return true;
}

const UObject* UWxSkillCutsceneComponent::GetStreamingSourceOwner() const
{
	return this;
}

bool UWxSkillCutsceneComponent::IsSceneReady() const
{
	if (bStreamingSourceRegistered && GFrameCounter <= StreamingSourceFrame + 1)
	{
		return false;
	}
	const UWorldPartitionSubsystem* Partition = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	return !Partition || Partition->IsStreamingCompleted(this);
}
