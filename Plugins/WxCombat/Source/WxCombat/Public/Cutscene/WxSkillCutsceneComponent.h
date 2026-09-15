// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Misc/Optional.h"
#include "WxSkillCutsceneComponent.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
class ULevelSequence;
class USkeletalMeshComponent;
class ALevelSequenceActor;
struct FStreamableHandle;

UENUM()
enum class EWxSkillCutscenePhase : uint8
{
	Idle,
	Playing,
};

USTRUCT()
struct FWxSkillCutsceneSession
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Id = 0;

	UPROPERTY()
	TSoftObjectPtr<ULevelSequence> Sequence;

	UPROPERTY()
	TObjectPtr<AActor> Avatar;

	UPROPERTY()
	FTransform Origin = FTransform::Identity;

	UPROPERTY()
	double Duration = 0.0;
};

USTRUCT()
struct FWxSkillCutsceneState
{
	GENERATED_BODY()

	UPROPERTY()
	FWxSkillCutsceneSession Session;

	UPROPERTY()
	EWxSkillCutscenePhase Phase = EWxSkillCutscenePhase::Idle;
};

USTRUCT()
struct FWxSkillCutsceneCompletion
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Id = 0;

	UPROPERTY()
	TObjectPtr<AActor> Avatar;

	UPROPERTY()
	bool bCancelled = false;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxSkillCutsceneEnded, AActor*, uint32, bool);

enum class EWxSkillCutsceneLocalPhase : uint8
{
	Idle,
	Preparing,
	Playing,
};

USTRUCT()
struct FWxSkillCutsceneLocalPlayback
{
	GENERATED_BODY()

	/** 서버 정상 완료 후에도 로컬 재생과 로딩에 필요한 참조를 보존한다. */
	UPROPERTY(Transient)
	FWxSkillCutsceneSession Session;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	EWxSkillCutsceneLocalPhase Phase = EWxSkillCutsceneLocalPhase::Idle;
	TSharedPtr<FStreamableHandle> SequenceLoad;
	double PreparationDeadline = 0.0;
	double StartTime = 0.0;
	TWeakObjectPtr<USkeletalMeshComponent> PoseMesh;
	bool bPreviousOnlyAllowAutonomousTickPose = false;
};

struct FWxSkillCutsceneServerExecution
{
	TWeakObjectPtr<UGameplayAbility> Reservation;
	float RequestedDilation = 1.f;
	double StartTime = 0.0;
};

/** 값이 있을 때만 컷신이 변경한 상태를 복원한다. */
struct FWxSkillCutsceneServerRestore
{
	TOptional<float> TimeDilation;
	TOptional<bool> AvatarAlwaysRelevant;
	TWeakObjectPtr<UAbilitySystemComponent> InvincibleASC;
	FActiveGameplayEffectHandle InvincibleHandle;
};

/** GameState가 같은 장면을 전달하고 각 머신이 로딩 후 처음부터 재생한다. */
UCLASS()
class WXCOMBAT_API UWxSkillCutsceneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxSkillCutsceneComponent();
	static UWxSkillCutsceneComponent* Get(const UWorld* World);

	bool IsBusy() const;
	bool IsForAvatar(const AActor* Avatar) const;
	bool Reserve(UGameplayAbility* Requester);
	bool Start(UGameplayAbility* Requester, ULevelSequence* Sequence, float Dilation);
	void Cancel(UGameplayAbility* Requester);
	uint32 GetSessionId() const;
	double GetPlaybackSeconds() const;

	FWxSkillCutsceneEnded OnCutsceneEnded;

	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_State();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSessionStarted(const FWxSkillCutsceneSession& Session);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSessionEnded(const FWxSkillCutsceneCompletion& Session);

	void QueueLocalSession(const FWxSkillCutsceneSession& Session);
	void RefreshSessionAvatar(uint32 SessionId, AActor* Avatar);

	UFUNCTION()
	void HandleLocalSequenceFinished();

	void BeginNextLocalSession();

	void PrepareLocalPlayer();
	void StartLocalPlayer();
	void EnableLocalMeshPoseTick();
	void CleanupLocalPlayer();
	void NotifyReadyClientCompletions();
	void Finish(bool bCancelled);
	void RestoreServerState();
	bool HasAuthority() const;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FWxSkillCutsceneState State;

	UPROPERTY(Transient)
	FWxSkillCutsceneLocalPlayback LocalPlayback;

	UPROPERTY(Transient)
	TArray<FWxSkillCutsceneSession> PendingLocalSessions;

	/** 서버 종료가 먼저 도착해도 해당 로컬 재생과 AnimBP 복원이 끝날 때까지 보관한다. */
	UPROPERTY(Transient)
	TArray<FWxSkillCutsceneCompletion> PendingClientCompletions;

	FWxSkillCutsceneServerExecution ServerExecution;
	FWxSkillCutsceneServerRestore ServerRestore;

	uint32 ObservedSessionId = 0;
	uint32 LastReceivedEndId = 0;
};
