// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WxSkillCutsceneComponent.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
class ULevelSequence;
class USkeletalMeshComponent;
class ALevelSequenceActor;
struct FStreamableHandle;

/** 모든 머신이 같은 장면을 재생하는 데 필요한 정보. */
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
};

USTRUCT()
struct FWxSkillCutsceneState
{
	GENERATED_BODY()

	UPROPERTY()
	FWxSkillCutsceneSession Session;

	UPROPERTY()
	bool bPlaying = false;

	/** 강제 취소로 끝났는지. 각 머신은 이 값으로 로컬 재생을 끊을지 끝까지 갈지 정한다. */
	UPROPERTY()
	bool bCancelled = false;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FWxSkillCutsceneEnded, AActor*, bool);

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

	/** 서버가 먼저 끝내도 로컬 재생과 종료 통지에 필요하므로 사본을 들고 있는다. */
	UPROPERTY(Transient)
	FWxSkillCutsceneSession Session;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	EWxSkillCutsceneLocalPhase Phase = EWxSkillCutsceneLocalPhase::Idle;
	TSharedPtr<FStreamableHandle> SequenceLoad;
	TWeakObjectPtr<USkeletalMeshComponent> PoseMesh;
	bool bPreviousOnlyAllowAutonomousTickPose = false;
};

/** 컷신이 바꾼 서버 상태. 종료에서 되돌린다. */
struct FWxSkillCutsceneServerState
{
	TWeakObjectPtr<UGameplayAbility> Owner;
	TWeakObjectPtr<UAbilitySystemComponent> InvincibleASC;
	FActiveGameplayEffectHandle InvincibleHandle;
	double Duration = 0.0;
	double StartTime = 0.0;
	float AppliedDilation = 0.f;
	bool bAvatarWasAlwaysRelevant = false;
};

/**
 * GameState가 같은 장면을 전달하고 각 머신이 로딩 후 처음부터 재생한다.
 *
 * 시퀀스 저작 규약 2가지:
 * - Binding Tag "Player"가 시전자 AvatarActor를 가리킨다. 그 바인딩의 레퍼런스 액터는 월드 원점에 두고 트랜스폼 트랙을 두지 않는다 — 원점이 아바타의 메시로 옮겨오므로, 트랙이 남아 있으면 아바타를 메시 자리까지 끌어내린다.
 * - 시전자 애니메이션 섹션은 Force Custom Mode를 켠다. 그래야 시퀀서가 대기/이동 AnimBP 대신 시전자 포즈를 소유한다 — 꺼두면 원격 머신에서만 시전자가 대기 자세로 남는다.
 */
UCLASS()
class WXCOMBAT_API UWxSkillCutsceneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxSkillCutsceneComponent();
	static UWxSkillCutsceneComponent* Get(const UWorld* World);

	bool IsBusy() const;
	bool Start(UGameplayAbility* Requester, ULevelSequence* Sequence, float Dilation);
	void Cancel(UGameplayAbility* Requester);

	FWxSkillCutsceneEnded OnCutsceneEnded;

	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_State();

	UFUNCTION()
	void HandleLocalSequenceFinished();

	void UpdateSession();
	void NotifyEnded(bool bCancelled);
	void PrepareLocalPlayer();
	void StartLocalPlayer();
	void CleanupLocalPlayer();
	void Finish(bool bCancelled);
	bool HasAuthority() const;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FWxSkillCutsceneState State;

	UPROPERTY(Transient)
	FWxSkillCutsceneLocalPlayback LocalPlayback;

	FWxSkillCutsceneServerState ServerState;

	/** 세션 하나당 종료를 한 번만 알린다. 첫 세션 전에는 알릴 것이 없다. */
	bool bEndNotified = true;
};
