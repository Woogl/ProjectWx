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
 * Level Sequence 컷신 재생 AbilityTask.
 * Global Time Dilation으로 게임 월드를 정지시키고, 시퀀스만 정상 속도로 재생한다.
 * AvatarActor의 Transform을 TransformOrigin으로 사용하며, BindingTag로 액터를 리바인딩한다.
 * 시퀀스 종료 시 Time Dilation을 복원하고 OnCompleted를 브로드캐스트한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_PlaySkillCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	/**
	 * @param InLevelSequence         재생할 Level Sequence 에셋
	 * @param InGlobalTimeDilation    컷신 재생 중 적용할 Global Time Dilation
	 *
	 * AvatarActor를 시퀀스에 리바인딩하려면 LevelSequence 에디터에서 Binding Tag를 "Player"로 설정해야 한다.
	 */
	static UWxAbilityTask_PlaySkillCutscene* CreateTask(UGameplayAbility* OwningAbility, ULevelSequence* InLevelSequence, float InGlobalTimeDilation);

	UPROPERTY()
	FWxOnCutsceneCompleted OnCompleted;

	UPROPERTY()
	FWxOnCutsceneCancelled OnCancelled;

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleSequenceFinished();

	void AddInvincibleTag();
	void RemoveInvincibleTag();
	void RestoreTimeDilation();
	void CleanupSequenceActor();

	UPROPERTY()
	TObjectPtr<ULevelSequence> LevelSequence;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	float GlobalTimeDilation = 1.f;
	float OriginalTimeDilation = 1.f;
};
