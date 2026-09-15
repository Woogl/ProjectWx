// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WorldPartition/WorldPartitionStreamingSource.h"
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
struct FWxSkillCutsceneState
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Id = 0;

	UPROPERTY()
	EWxSkillCutscenePhase Phase = EWxSkillCutscenePhase::Idle;

	UPROPERTY()
	TSoftObjectPtr<ULevelSequence> Sequence;

	UPROPERTY()
	TObjectPtr<AActor> Avatar;

	UPROPERTY()
	FTransform Origin = FTransform::Identity;

	UPROPERTY()
	double Duration = 0.0;

	UPROPERTY()
	bool bCancelled = false;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxSkillCutsceneEnded, AActor*, uint32, bool);

/** GameState가 같은 장면을 전달하고 각 머신이 로딩 후 처음부터 재생한다. */
UCLASS()
class WXCOMBAT_API UWxSkillCutsceneComponent : public UActorComponent, public IWorldPartitionStreamingSourceProvider
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
	virtual bool GetStreamingSource(FWorldPartitionStreamingSource& OutSource) const override;
	virtual const UObject* GetStreamingSourceOwner() const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_State();

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
	bool IsSceneReady() const;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FWxSkillCutsceneState State;

	/** 서버 정상 완료 후에도 로컬 재생과 로딩에 필요한 참조를 보존한다. */
	UPROPERTY(Transient)
	FWxSkillCutsceneState LocalState;

	UPROPERTY(Transient)
	TArray<FWxSkillCutsceneState> PendingLocalSessions;

	/** 서버 종료가 먼저 도착해도 해당 로컬 재생과 AnimBP 복원이 끝날 때까지 보관한다. */
	UPROPERTY(Transient)
	TArray<FWxSkillCutsceneState> PendingClientCompletions;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> LocalSequenceActor;

	TWeakObjectPtr<UGameplayAbility> Reservation;
	TWeakObjectPtr<USkeletalMeshComponent> LocalPoseMesh;
	bool bPreviousOnlyAllowAutonomousTickPose = false;
	TWeakObjectPtr<UAbilitySystemComponent> InvincibleASC;
	FActiveGameplayEffectHandle InvincibleHandle;
	TSharedPtr<FStreamableHandle> SequenceLoad;
	uint32 ObservedSessionId = 0;
	uint32 LastReceivedEndId = 0;
	uint64 StreamingSourceFrame = 0;
	double PreparationDeadline = 0.0;
	double LocalPlaybackStartTime = 0.0;
	double ServerPlaybackStartTime = 0.0;
	float RequestedDilation = 1.f;
	float PreviousDilation = 1.f;
	bool bDilationApplied = false;
	bool bAvatarWasAlwaysRelevant = false;
	bool bAvatarRelevancyChanged = false;
	bool bLocalActive = false;
	bool bLocalPlaying = false;
	bool bStreamingSourceRegistered = false;
};
