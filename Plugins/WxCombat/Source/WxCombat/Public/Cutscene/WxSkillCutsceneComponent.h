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

/** 모든 머신이 같은 장면을 재생하는 데 필요한 정보. */
USTRUCT()
struct FWxSkillCutsceneSession
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Id = 0;

	UPROPERTY()
	TObjectPtr<ULevelSequence> Sequence;

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

	/** 각 머신은 이 값으로 로컬 재생을 끊을지 끝까지 갈지 정한다. */
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

	/** State가 다음 세션으로 덮여도 이 머신이 재생·통지 중인 세션을 가리키도록 사본을 둔다. */
	UPROPERTY(Transient)
	FWxSkillCutsceneSession Session;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	EWxSkillCutsceneLocalPhase Phase = EWxSkillCutsceneLocalPhase::Idle;
	TWeakObjectPtr<USkeletalMeshComponent> PoseMesh;
	bool bPreviousOnlyAllowAutonomousTickPose = false;
};

/** 컷신이 바꾼 서버 상태. 종료에서 되돌린다. */
struct FWxSkillCutsceneServerState
{
	TWeakObjectPtr<UGameplayAbility> Owner;
	TWeakObjectPtr<UAbilitySystemComponent> InvincibleASC;
	FActiveGameplayEffectHandle InvincibleHandle;

	/** 월드 오디오 시각. 로컬 재생과 무관하게 이 시각에 세션을 끝낸다. */
	double EndTime = 0.0;

	float AppliedDilation = 0.f;
	bool bAvatarWasAlwaysRelevant = false;
};

/**
 * GameState가 같은 장면을 전달하고, 각 머신은 받으면 처음부터 자기 끝까지 재생한다.
 * 세션 수명(월드 배율·무적·관련성)은 서버가 시퀀스 길이로만 정한다.
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

	/**
	 * 컷신 동안 Groom 시뮬만 정상 속도로 돌린다. 1.0 이면 원래대로 되돌린다.
	 *
	 * 배율을 걸면 엔진이 시뮬을 배치에서 solo 로 옮겨 소유 액터의 CustomTimeDilation 과 틱 그룹에 함께 묶인다.
	 * 그래서 호출부가 월드·액터 배율을 모두 상쇄하고 매 틱 갱신한다.
	 */
	void SetGroomTimeDilation(float Dilation);

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
