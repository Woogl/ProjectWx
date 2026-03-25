// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_PlayCutscene.generated.h"

class ALevelSequenceActor;
class ULevelSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCancelled);

/**
 * Level Sequence 컷신 재생 AbilityTask.
 * Global Time Dilation으로 게임 월드를 정지시키고, 시퀀스만 정상 속도로 재생한다.
 * 시퀀스 종료 시 Time Dilation을 복원하고 OnCompleted를 브로드캐스트한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_PlayCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_PlayCutscene* CreateTask(UGameplayAbility* OwningAbility, ULevelSequence* InLevelSequence);

	UPROPERTY()
	FWxOnCutsceneCompleted OnCompleted;

	UPROPERTY()
	FWxOnCutsceneCancelled OnCancelled;

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleSequenceFinished();
	void AddInvincibleTag();
	void RemoveInvincibleTag();
	void RestoreTimeDilation();
	void CleanupSequenceActor();

	UPROPERTY()
	TObjectPtr<ULevelSequence> LevelSequence;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	float OriginalTimeDilation = 1.f;
	bool bTimeDilationModified = false;
	bool bInvincibleTagAdded = false;
};
