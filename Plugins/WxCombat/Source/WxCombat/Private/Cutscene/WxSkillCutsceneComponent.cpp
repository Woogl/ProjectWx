// Copyright Woogle. All Rights Reserved.

#include "Cutscene/WxSkillCutsceneComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/WorldSettings.h"
#include "GroomComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneTimeController.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "WxCombatModule.h"

namespace
{
	/**
	 * 월드 오디오 시각(배율은 받지 않고 일시정지에는 멈춘다)으로 직접 진행해 월드 배율과 시퀀스의 Clock Source에 의존하지 않는다.
	 * 엔진의 외부 시계(FMovieSceneTimeController_ExternalClock)는 월드 배율이 곱해진 PlayRate를 다시 반영하므로 0.001배 월드에서는 쓸 수 없다.
	 */
	class FWxSkillCutsceneClock : public FMovieSceneTimeController
	{
	public:
		explicit FWxSkillCutsceneClock(const UWorld* InWorld);

	protected:
		virtual void OnStartPlaying(const FQualifiedFrameTime& InStartTime) override;
		virtual FFrameTime OnRequestCurrentTime(const FQualifiedFrameTime& InCurrentTime, float InPlayRate) override;

	private:
		TWeakObjectPtr<const UWorld> World;
		double StartSeconds = 0.0;
	};

	FWxSkillCutsceneClock::FWxSkillCutsceneClock(const UWorld* InWorld)
		: World(InWorld)
	{
	}

	void FWxSkillCutsceneClock::OnStartPlaying(const FQualifiedFrameTime& InStartTime)
	{
		if (const UWorld* CurrentWorld = World.Get())
		{
			StartSeconds = CurrentWorld->GetAudioTimeSeconds();
		}
	}

	FFrameTime FWxSkillCutsceneClock::OnRequestCurrentTime(const FQualifiedFrameTime& InCurrentTime, float InPlayRate)
	{
		const UWorld* CurrentWorld = World.Get();
		const TOptional<FQualifiedFrameTime> PlaybackStart = GetPlaybackStartTime();
		if (!CurrentWorld || !PlaybackStart.IsSet())
		{
			return InCurrentTime.Time;
		}

		// 경과를 상한으로 자르지 않는다. 엔진이 재생 범위 끝에서 스스로 멈춘다.
		return PlaybackStart->ConvertTo(InCurrentTime.Rate) + InCurrentTime.Rate.AsFrameTime(CurrentWorld->GetAudioTimeSeconds() - StartSeconds);
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
	return State.bPlaying;
}

bool UWxSkillCutsceneComponent::Start(UGameplayAbility* Requester, ULevelSequence* Sequence, float Dilation)
{
	if (!HasAuthority() || !Requester || IsBusy())
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
	if (Sequence->FindBindingsByTag(TEXT("Player")).IsEmpty())
	{
		UE_LOG(LogWxCombat, Warning, TEXT("컷신 시퀀스 %s에 Binding Tag \"Player\"가 없다."), *Sequence->GetPathName());
		return false;
	}

	++State.Session.Id;
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
	// bCancelled는 직전 종료의 결과다. 여기서 지우면 두 전이가 한 복제 창에 겹쳤을 때 그 결과를 잃는다.
	State.bPlaying = true;

	ServerState.Owner = Requester;
	ServerState.EndTime = GetWorld()->GetAudioTimeSeconds() + Duration;

	UGameplayStatics::SetGlobalTimeDilation(this, FMath::Max(Dilation, 0.001f));

	// 엔진이 Min/MaxGlobalTimeDilation으로 클램프하므로, 해제 때 비교하려면 요청값이 아니라 실제로 박힌 값을 들고 있어야 한다.
	ServerState.AppliedDilation = UGameplayStatics::GetGlobalTimeDilation(this);

	ServerState.bAvatarWasAlwaysRelevant = Avatar->bAlwaysRelevant;
	Avatar->bAlwaysRelevant = true;
	Avatar->FlushNetDormancy();
	Avatar->ForceNetUpdate();

	if (UAbilitySystemComponent* ASC = Requester->GetAbilitySystemComponentFromActorInfo())
	{
		ServerState.InvincibleASC = ASC;
		ServerState.InvincibleHandle = ASC->ApplyGameplayEffectToSelf(GetDefault<UWxEffect_Invincible>(), Requester->GetAbilityLevel(), ASC->MakeEffectContext());
	}

	UpdateSession();
	GetOwner()->ForceNetUpdate();
	return true;
}

void UWxSkillCutsceneComponent::Cancel(UGameplayAbility* Requester)
{
	if (HasAuthority() && Requester && ServerState.Owner.Get() == Requester)
	{
		Finish(true);
	}
}

void UWxSkillCutsceneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWxSkillCutsceneComponent, State);
}

void UWxSkillCutsceneComponent::OnRep_State()
{
	UpdateSession();
}

void UWxSkillCutsceneComponent::UpdateSession()
{
	const bool bNewSession = State.Session.Id != LocalPlayback.Session.Id;
	if (bNewSession)
	{
		// 남아 있던 로컬 재생은 밀어내고, 그 세션의 종료를 먼저 알린다.
		// 직전 종료를 못 보고 이 시작만 받았을 수도 있으므로 결과는 State가 들고 있는 값을 쓴다.
		CleanupLocalPlayer();
		NotifyEnded(State.bCancelled);
	}

	// 미해석이던 시전자 참조가 풀리면 엔진이 OnRep을 다시 부르므로, 사본은 매번 갱신한다.
	LocalPlayback.Session = State.Session;

	if (bNewSession)
	{
		// 시작을 못 본 채 끝난 세션은 통지 없이 입양만 한다. 그 세션을 기다리던 궁극기는 서버가 복제하는 어빌리티 종료로 닫힌다.
		// 여기서 알리면 커밋 실패로 한 호출 안에서 열리고 닫힌 세션의 취소가 직후의 재발동을 끊는다.
		bEndNotified = !State.bPlaying;
		if (State.bPlaying && GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			LocalPlayback.Phase = EWxSkillCutsceneLocalPhase::Preparing;
		}
	}

	if (State.bPlaying)
	{
		return;
	}

	// 서버가 끝냈다. 강제 취소가 아니면 이미 재생 중인 장면은 자기 끝까지 간다.
	if (LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Playing && !State.bCancelled)
	{
		return;
	}

	// 준비 중이었다면 여기서 접는다. 서버가 끝냈으면 더 기다릴 이유가 없다.
	CleanupLocalPlayer();
	NotifyEnded(State.bCancelled);
}

void UWxSkillCutsceneComponent::NotifyEnded(bool bCancelled)
{
	if (bEndNotified)
	{
		return;
	}

	bEndNotified = true;
	OnCutsceneEnded.Broadcast(LocalPlayback.Session.Avatar, bCancelled);
}

void UWxSkillCutsceneComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HasAuthority() && State.bPlaying)
	{
		if (!ServerState.Owner.IsValid() || !IsValid(State.Session.Avatar))
		{
			Finish(true);
			return;
		}

		// 세션은 길이로만 끝낸다. 이 머신의 로컬 재생도 다른 머신처럼 따로 자기 끝까지 간다.
		// 바꾼 배율은 다음 프레임 델타부터 들어가므로, 이번 틱의 Groom 역배율은 옛 값 그대로 두고 반환한다.
		if (GetWorld()->GetAudioTimeSeconds() >= ServerState.EndTime)
		{
			Finish(false);
			return;
		}
	}

	PrepareLocalPlayer();

	if (LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Playing && IsValid(LocalPlayback.Session.Avatar))
	{
		// 월드·액터 배율 모두 컷신 도중 바뀐다 — 서버가 먼저 끝내거나, 히트스톱이 액터를 멈추거나, 다른 슬로모션이 끼어든다.
		// 매 틱 현재값을 상쇄하지 않으면 남은 역배율이 시뮬을 발산시킨다.
		const float Combined = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation() * LocalPlayback.Session.Avatar->CustomTimeDilation;
		SetGroomTimeDilation(1.f / FMath::Max(Combined, UE_KINDA_SMALL_NUMBER));
	}
}

void UWxSkillCutsceneComponent::PrepareLocalPlayer()
{
	// 참조가 늦게 풀리면 엔진이 OnRep을 다시 불러 사본을 채우고, 끝내 안 풀리면 서버 종료가 준비를 접는다.
	const FWxSkillCutsceneSession& Session = LocalPlayback.Session;
	if (LocalPlayback.Phase != EWxSkillCutsceneLocalPhase::Preparing || !Session.Sequence || !IsValid(Session.Avatar))
	{
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bDisableMovementInput = true;
	Settings.bDisableLookAtInput = true;
	Settings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceRestoreState;
	ALevelSequenceActor* NewActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Session.Sequence, Settings, NewActor);
	LocalPlayback.SequenceActor = NewActor;
	UDefaultLevelSequenceInstanceData* InstanceData = NewActor ? Cast<UDefaultLevelSequenceInstanceData>(NewActor->DefaultInstanceData) : nullptr;
	if (!Player || !InstanceData)
	{
		// 이 머신만 재생을 건너뛴다. 종료는 서버가 길이로 끝낼 때 통지된다.
		UE_LOG(LogWxCombat, Warning, TEXT("컷신 %u: 로컬 시퀀스 플레이어 준비 실패."), Session.Id);
		CleanupLocalPlayer();
		return;
	}

	NewActor->bOverrideInstanceData = true;
	InstanceData->TransformOrigin = Session.Origin;
	TArray<AActor*> Actors;
	Actors.Add(Session.Avatar);
	NewActor->SetBindingByTag(TEXT("Player"), Actors, false);
	Player->SetTimeController(MakeShared<FWxSkillCutsceneClock>(GetWorld()));

	StartLocalPlayer();
}

void UWxSkillCutsceneComponent::StartLocalPlayer()
{
	if (ACharacter* Character = Cast<ACharacter>(LocalPlayback.Session.Avatar))
	{
		Character->StopAnimMontage();
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			LocalPlayback.PoseMesh = Mesh;
			LocalPlayback.bPreviousOnlyAllowAutonomousTickPose = Mesh->bOnlyAllowAutonomousTickPose;

			// 서버의 원격 시전자는 평소 이동 패킷이 포즈를 갱신한다. 컷신은 일반 메시 틱에서도 평가되어야 한다.
			Mesh->bOnlyAllowAutonomousTickPose = false;
		}
	}

	LocalPlayback.Phase = EWxSkillCutsceneLocalPhase::Playing;
	LocalPlayback.SequenceActor->GetSequencePlayer()->OnFinished.AddDynamic(this, &UWxSkillCutsceneComponent::HandleLocalSequenceFinished);
	LocalPlayback.SequenceActor->GetSequencePlayer()->Play();
}

void UWxSkillCutsceneComponent::HandleLocalSequenceFinished()
{
	// 시퀀서가 사전 상태를 되돌린 뒤라야 후속 몽타주가 포즈를 잡는다.
	CleanupLocalPlayer();
	UpdateSession();
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
	if (LocalPlayback.Phase == EWxSkillCutsceneLocalPhase::Playing)
	{
		SetGroomTimeDilation(1.f);
	}
	LocalPlayback.PoseMesh.Reset();
	LocalPlayback.Phase = EWxSkillCutsceneLocalPhase::Idle;
}

void UWxSkillCutsceneComponent::SetGroomTimeDilation(float Dilation)
{
	AActor* Avatar = LocalPlayback.Session.Avatar;
	if (!IsValid(Avatar))
	{
		return;
	}

	const bool bSolo = !FMath::IsNearlyEqual(Dilation, 1.f);
	TInlineComponentArray<UGroomComponent*> Grooms(Avatar);
	for (UGroomComponent* Groom : Grooms)
	{
		for (UNiagaraComponent* Simulation : Groom->NiagaraComponents)
		{
			if (!Simulation)
			{
				continue;
			}

			// 1.0이 아닌 배율은 엔진이 solo 모드로 돌려 컴포넌트 틱에서 평가한다.
			Simulation->SetCustomTimeDilation(Dilation);

			// 그 전환은 틱 그룹을 갱신하지 않아 생성자 기본값(TG_PrePhysics)이 남는다.
			// 배치에서 돌던 자리로 옮겨 그 프레임의 포즈가 선 뒤에 시뮬되게 한다.
			if (bSolo)
			{
				Simulation->PrimaryComponentTick.TickGroup = TG_EndPhysics;
				Simulation->PrimaryComponentTick.EndTickGroup = TG_LastDemotable;
			}
		}
	}
}

void UWxSkillCutsceneComponent::Finish(bool bCancelled)
{
	// 시작 때 박은 값이 그대로일 때만 되돌린다. 컷신 도중 끊긴 다른 슬로우 타임의 배율을 되살리지 않는다.
	if (ServerState.AppliedDilation > 0.f && FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(this), ServerState.AppliedDilation))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	}
	if (UAbilitySystemComponent* ASC = ServerState.InvincibleASC.Get())
	{
		ASC->RemoveActiveGameplayEffect(ServerState.InvincibleHandle);
	}
	if (IsValid(State.Session.Avatar))
	{
		State.Session.Avatar->bAlwaysRelevant = ServerState.bAvatarWasAlwaysRelevant;
		State.Session.Avatar->ForceNetUpdate();
	}
	ServerState = FWxSkillCutsceneServerState();

	State.bPlaying = false;
	State.bCancelled = bCancelled;

	// 마지막 상태에도 장면 정보를 남겨 늦게 해석되는 참조를 보존한다.
	GetOwner()->ForceNetUpdate();
	UpdateSession();
}

void UWxSkillCutsceneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 월드가 내려가는 중이므로 종료 콜백은 부르지 않는다.
	bEndNotified = true;
	if (HasAuthority() && State.bPlaying)
	{
		Finish(true);
	}
	CleanupLocalPlayer();
	Super::EndPlay(EndPlayReason);
}
