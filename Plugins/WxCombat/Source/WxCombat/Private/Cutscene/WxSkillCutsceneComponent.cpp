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
	return ServerExecution.Reservation.IsValid() || State.Phase != EWxSkillCutscenePhase::Idle;
}

bool UWxSkillCutsceneComponent::Reserve(UGameplayAbility* Requester)
{
	if (!HasAuthority() || !Requester || IsBusy())
	{
		return false;
	}
	ServerExecution.Reservation = Requester;
	++State.Session.Id;
	State.Session.Avatar = Requester->GetAvatarActorFromActorInfo();
	return true;
}

bool UWxSkillCutsceneComponent::Start(UGameplayAbility* Requester, ULevelSequence* Sequence, float Dilation)
{
	if (!HasAuthority() || !Requester || ServerExecution.Reservation.Get() != Requester || State.Phase != EWxSkillCutscenePhase::Idle)
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

	State.Session.Sequence = Sequence;
	State.Session.Avatar = Avatar;
	State.Session.Origin = Avatar->GetActorTransform();
	if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			State.Session.Origin = Mesh->GetComponentTransform();
		}
	}
	State.Session.Duration = Duration;
	ServerExecution.StartTime = 0.0;
	State.Phase = EWxSkillCutscenePhase::Playing;
	ServerExecution.RequestedDilation = FMath::Max(Dilation, 0.001f);
	ServerRestore.AvatarAlwaysRelevant = Avatar->bAlwaysRelevant;
	Avatar->bAlwaysRelevant = true;
	Avatar->FlushNetDormancy();
	Avatar->ForceNetUpdate();

	if (UAbilitySystemComponent* ASC = Requester->GetAbilitySystemComponentFromActorInfo())
	{
		ServerRestore.InvincibleASC = ASC;
		ServerRestore.InvincibleHandle = ASC->ApplyGameplayEffectToSelf(GetDefault<UWxEffect_Invincible>(), Requester->GetAbilityLevel(), ASC->MakeEffectContext());
	}

	MulticastSessionStarted(State.Session);
	GetOwner()->ForceNetUpdate();
	return true;
}

void UWxSkillCutsceneComponent::Cancel(UGameplayAbility* Requester)
{
	if (HasAuthority() && Requester && ServerExecution.Reservation.Get() == Requester)
	{
		Finish(true);
	}
}

uint32 UWxSkillCutsceneComponent::GetSessionId() const
{
	return State.Session.Id;
}

double UWxSkillCutsceneComponent::GetPlaybackSeconds() const
{
	return LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Playing
		? FMath::Clamp(FPlatformTime::Seconds() - LocalPlayback.StartTime, 0.0, LocalPlayback.Session.Duration) : 0.0;
}

void UWxSkillCutsceneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWxSkillCutsceneComponent, State);
}

void UWxSkillCutsceneComponent::OnRep_State()
{
	// 접속 시 이미 진행 중인 컷신은 스냅샷으로 준비한다. 이후 전이는 Reliable RPC가 전달한다.
	if (ObservedSessionId == 0 && State.Phase == EWxSkillCutscenePhase::Playing)
	{
		QueueLocalSession(State.Session);
	}
	RefreshSessionAvatar(State.Session.Id, State.Session.Avatar);
	NotifyReadyClientCompletions();
}

void UWxSkillCutsceneComponent::QueueLocalSession(const FWxSkillCutsceneSession& Session)
{
	if (Session.Id <= ObservedSessionId)
	{
		return;
	}
	ObservedSessionId = Session.Id;
	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		PendingLocalSessions.Add(Session);
		BeginNextLocalSession();
	}
}

void UWxSkillCutsceneComponent::RefreshSessionAvatar(uint32 SessionId, AActor* Avatar)
{
	if (!IsValid(Avatar))
	{
		return;
	}
	if (LocalPlayback.Session.Id == SessionId)
	{
		LocalPlayback.Session.Avatar = Avatar;
	}
	for (FWxSkillCutsceneSession& Pending : PendingLocalSessions)
	{
		if (Pending.Id == SessionId)
		{
			Pending.Avatar = Avatar;
		}
	}
	for (FWxSkillCutsceneCompletion& Pending : PendingClientCompletions)
	{
		if (Pending.Id == SessionId)
		{
			Pending.Avatar = Avatar;
		}
	}
}

void UWxSkillCutsceneComponent::MulticastSessionStarted_Implementation(const FWxSkillCutsceneSession& Session)
{
	if (Session.Id >= State.Session.Id)
	{
		State.Session = Session;
		State.Phase = EWxSkillCutscenePhase::Playing;
	}
	QueueLocalSession(Session);
	RefreshSessionAvatar(Session.Id, Session.Avatar);
}

void UWxSkillCutsceneComponent::MulticastSessionEnded_Implementation(const FWxSkillCutsceneCompletion& Session)
{
	if (HasAuthority())
	{
		return;
	}
	if (Session.Id >= State.Session.Id)
	{
		State.Session.Id = Session.Id;
		State.Session.Avatar = Session.Avatar;
		State.Phase = EWxSkillCutscenePhase::Idle;
	}
	RefreshSessionAvatar(Session.Id, Session.Avatar);
	if (Session.bCancelled)
	{
		for (int32 Index = PendingLocalSessions.Num() - 1; Index >= 0; --Index)
		{
			if (PendingLocalSessions[Index].Id == Session.Id)
			{
				PendingLocalSessions.RemoveAt(Index);
			}
		}
		if (LocalPlayback.Phase != EWxSkillCutsceneLocalPhase::Idle && LocalPlayback.Session.Id == Session.Id)
		{
			CleanupLocalPlayer();
		}
	}
	if (Session.Id > LastReceivedEndId)
	{
		LastReceivedEndId = Session.Id;
		FWxSkillCutsceneCompletion Completion = Session;
		if (!IsValid(Completion.Avatar) && LocalPlayback.Session.Id == Session.Id)
		{
			Completion.Avatar = LocalPlayback.Session.Avatar;
		}
		PendingClientCompletions.Add(Completion);
	}
	BeginNextLocalSession();
	NotifyReadyClientCompletions();
}

void UWxSkillCutsceneComponent::NotifyReadyClientCompletions()
{
	for (int32 Index = 0; Index < PendingClientCompletions.Num();)
	{
		const FWxSkillCutsceneCompletion Completion = PendingClientCompletions[Index];
		bool bWaitingForLocal = LocalPlayback.Phase != EWxSkillCutsceneLocalPhase::Idle && LocalPlayback.Session.Id == Completion.Id;
		for (const FWxSkillCutsceneSession& Pending : PendingLocalSessions)
		{
			bWaitingForLocal |= Pending.Id == Completion.Id;
		}
		if (!IsValid(Completion.Avatar) || (!Completion.bCancelled && bWaitingForLocal))
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
	if (LocalPlayback.Phase != EWxSkillCutsceneLocalPhase::Idle || PendingLocalSessions.IsEmpty())
	{
		return;
	}
	LocalPlayback.Session = PendingLocalSessions[0];
	PendingLocalSessions.RemoveAt(0);
	LocalPlayback.Phase = EWxSkillCutsceneLocalPhase::Preparing;
	LocalPlayback.PreparationDeadline = FPlatformTime::Seconds() + 20.0;
}

void UWxSkillCutsceneComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	if (HasAuthority() && State.Phase != EWxSkillCutscenePhase::Idle)
	{
		if (!ServerExecution.Reservation.IsValid() || !IsValid(State.Session.Avatar))
		{
			Finish(true);
			return;
		}
		if (GetWorld()->GetNetMode() == NM_DedicatedServer && ServerExecution.StartTime == 0.0)
		{
			ServerRestore.TimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
			UGameplayStatics::SetGlobalTimeDilation(this, ServerExecution.RequestedDilation);
			ServerExecution.StartTime = FPlatformTime::Seconds();
		}
		if (GetWorld()->GetNetMode() == NM_DedicatedServer && ServerExecution.StartTime > 0.0 && FPlatformTime::Seconds() - ServerExecution.StartTime >= State.Session.Duration)
		{
			Finish(false);
			return;
		}
	}
	BeginNextLocalSession();
	if (LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Idle)
	{
		return;
	}
	if (LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Preparing && FPlatformTime::Seconds() >= LocalPlayback.PreparationDeadline)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("컷신 %u: 로컬 준비 20초 시간 초과."), LocalPlayback.Session.Id);
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
	if (LocalPlayback.Phase != EWxSkillCutsceneLocalPhase::Preparing)
	{
		return;
	}
	if (!LocalPlayback.Session.Sequence.Get() && !LocalPlayback.SequenceLoad)
	{
		LocalPlayback.SequenceLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(LocalPlayback.Session.Sequence.ToSoftObjectPath());
	}
	if (LocalPlayback.SequenceLoad && !LocalPlayback.SequenceLoad->HasLoadCompleted())
	{
		return;
	}
	bool bFailed = LocalPlayback.Session.Sequence.Get() == nullptr;
	if (!bFailed && !IsValid(LocalPlayback.Session.Avatar))
	{
		return;
	}
	if (!bFailed && !LocalPlayback.SequenceActor)
	{
		FMovieSceneSequencePlaybackSettings Settings;
		Settings.bDisableMovementInput = true;
		Settings.bDisableLookAtInput = true;
		Settings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceRestoreState;
		ALevelSequenceActor* NewActor = nullptr;
		ULevelSequence* PlaybackSequence = CreatePlayerCutsceneSequence(LocalPlayback.Session.Sequence.Get(), this);
		ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), PlaybackSequence, Settings, NewActor);
		LocalPlayback.SequenceActor = NewActor;
		bFailed = !Player || !NewActor || NewActor->FindNamedBindings(TEXT("Player")).IsEmpty();
		if (!bFailed)
		{
			NewActor->bOverrideInstanceData = true;
			UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(NewActor->DefaultInstanceData);
			bFailed = InstanceData == nullptr;
			if (InstanceData)
			{
				InstanceData->TransformOrigin = LocalPlayback.Session.Origin;
				TArray<AActor*> Actors;
				Actors.Add(LocalPlayback.Session.Avatar);
				NewActor->SetBindingByTag(TEXT("Player"), Actors, false);
				Player->SetTimeController(MakeShared<FWxSkillCutsceneClock>(this, Player->GetStartTime().AsSeconds()));
			}
		}
	}
	if (bFailed)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("컷신 %u: 로컬 시퀀스 또는 Player 바인딩 준비 실패."), LocalPlayback.Session.Id);
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
	if (LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Playing || !LocalPlayback.SequenceActor)
	{
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(LocalPlayback.Session.Avatar))
	{
		Character->StopAnimMontage();
	}
	EnableLocalMeshPoseTick();
	LocalPlayback.StartTime = FPlatformTime::Seconds();
	LocalPlayback.Phase = EWxSkillCutsceneLocalPhase::Playing;
	if (HasAuthority())
	{
		ServerRestore.TimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		UGameplayStatics::SetGlobalTimeDilation(this, ServerExecution.RequestedDilation);
	}
	LocalPlayback.SequenceActor->GetSequencePlayer()->OnFinished.AddDynamic(this, &UWxSkillCutsceneComponent::HandleLocalSequenceFinished);
	LocalPlayback.SequenceActor->GetSequencePlayer()->Play();
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
	const ACharacter* Character = Cast<ACharacter>(LocalPlayback.Session.Avatar);
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || LocalPlayback.PoseMesh.IsValid())
	{
		return;
	}

	LocalPlayback.PoseMesh = Mesh;
	LocalPlayback.bPreviousOnlyAllowAutonomousTickPose = Mesh->bOnlyAllowAutonomousTickPose;
	// 서버의 원격 시전자는 평소 이동 패킷이 포즈를 갱신한다. 컷신은 일반 메시 틱에서도 평가되어야 한다.
	Mesh->bOnlyAllowAutonomousTickPose = false;
}

void UWxSkillCutsceneComponent::CleanupLocalPlayer()
{
	if (LocalPlayback.SequenceActor)
	{
		if (ULevelSequencePlayer* Player = LocalPlayback.SequenceActor->GetSequencePlayer())
		{
			Player->OnFinished.RemoveDynamic(this, &UWxSkillCutsceneComponent::HandleLocalSequenceFinished);
			Player->Stop();
			Player->RestoreState();
		}
		LocalPlayback.SequenceActor->Destroy();
		LocalPlayback.SequenceActor = nullptr;
	}
	if (USkeletalMeshComponent* Mesh = LocalPlayback.PoseMesh.Get())
	{
		Mesh->bOnlyAllowAutonomousTickPose = LocalPlayback.bPreviousOnlyAllowAutonomousTickPose;
	}
	LocalPlayback.PoseMesh.Reset();
	LocalPlayback.SequenceLoad.Reset();
	LocalPlayback.Phase = EWxSkillCutsceneLocalPhase::Idle;
	LocalPlayback.StartTime = 0.0;
	NotifyReadyClientCompletions();
}

void UWxSkillCutsceneComponent::RestoreServerState()
{
	if (ServerRestore.TimeDilation.IsSet())
	{
		UGameplayStatics::SetGlobalTimeDilation(this, ServerRestore.TimeDilation.GetValue());
		ServerRestore.TimeDilation.Reset();
	}
	if (UAbilitySystemComponent* ASC = ServerRestore.InvincibleASC.Get())
	{
		ASC->RemoveActiveGameplayEffect(ServerRestore.InvincibleHandle);
	}
	ServerRestore.InvincibleHandle.Invalidate();
	ServerRestore.InvincibleASC.Reset();
	if (ServerRestore.AvatarAlwaysRelevant.IsSet() && IsValid(State.Session.Avatar))
	{
		State.Session.Avatar->bAlwaysRelevant = ServerRestore.AvatarAlwaysRelevant.GetValue();
		State.Session.Avatar->ForceNetUpdate();
	}
	ServerRestore.AvatarAlwaysRelevant.Reset();
}

void UWxSkillCutsceneComponent::Finish(bool bCancelled)
{
	const uint32 FinishedId = State.Session.Id;
	RestoreServerState();
	CleanupLocalPlayer();
	ServerExecution.Reservation.Reset();
	PendingLocalSessions.Reset();
	State.Phase = EWxSkillCutscenePhase::Idle;
	FWxSkillCutsceneCompletion Completion;
	Completion.Id = FinishedId;
	Completion.Avatar = State.Session.Avatar;
	Completion.bCancelled = bCancelled;
	MulticastSessionEnded(Completion);
	// 마지막 상태에도 장면 정보를 남겨 늦게 해석되는 참조를 보존한다.
	GetOwner()->ForceNetUpdate();
	OnCutsceneEnded.Broadcast(State.Session.Avatar, FinishedId, bCancelled);
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
