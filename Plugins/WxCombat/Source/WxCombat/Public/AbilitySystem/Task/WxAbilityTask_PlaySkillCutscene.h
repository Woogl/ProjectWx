// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_PlaySkillCutscene.generated.h"

class ALevelSequenceActor;
class ULevelSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCancelled);

/**
 * Global Time Dilation으로 게임 월드를 정지시키고 시퀀스만 정상 속도로 재생하며, AvatarActor의 스켈레탈 메시가 놓인 자리를 시퀀스 원점으로 쓴다.
 *
 * 배율은 서버에서만 걸고, 클라이언트에는 엔진의 WorldSettings 복제로 도착한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_PlaySkillCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	/**
	 * AvatarActor를 시퀀스에 리바인딩하려면 LevelSequence 에디터에서 Binding Tag를 "Player"로 설정해야 한다.
	 * 그 바인딩의 레퍼런스 액터는 월드 원점에 두고 트랜스폼 트랙을 두지 않는다 — 원점이 아바타의 메시로 옮겨오므로, 트랙이 남아 있으면 아바타를 메시 자리까지 끌어내린다.
	 */
	static UWxAbilityTask_PlaySkillCutscene* CreateTask(UGameplayAbility* OwningAbility, ULevelSequence* InLevelSequence, float InGlobalTimeDilation);

	UPROPERTY()
	FWxOnCutsceneCompleted OnCompleted;

	UPROPERTY()
	FWxOnCutsceneCancelled OnCancelled;

	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	UFUNCTION()
	void HandleSequenceFinished();

	/** 그 사이 다른 요청이 배율을 가져갔다면 아무것도 하지 않는다. */
	void ClearTimeDilation();

	void CleanupSequenceActor();

	UPROPERTY()
	TObjectPtr<ULevelSequence> LevelSequence;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	float GlobalTimeDilation = 1.f;

	/** 0이면 이 태스크가 배율을 걸지 않은 상태다. */
	float AppliedDilation = 0.f;
};
