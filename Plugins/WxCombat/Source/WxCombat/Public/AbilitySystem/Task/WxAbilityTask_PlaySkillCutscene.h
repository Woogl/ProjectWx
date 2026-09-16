// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_PlaySkillCutscene.generated.h"

class UWxSkillCutsceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCancelled);

/**
 * 시전자의 공용 컷신이 끝나기를 기다린다. 재생은 GameState의 UWxSkillCutsceneComponent가 소유하며, 이 태스크의 수명이 그 컷신의 수명을 쥔다.
 * 각 머신은 자기 재생이 끝난 시점에 통지를 받으므로, 후속 몽타주는 시퀀서가 포즈를 놓은 뒤에 시작한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_PlaySkillCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_PlaySkillCutscene* CreateTask(UGameplayAbility* OwningAbility);

	UPROPERTY()
	FWxOnCutsceneCompleted OnCompleted;

	UPROPERTY()
	FWxOnCutsceneCancelled OnCancelled;

	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	void HandleCutsceneEnded(AActor* Avatar, bool bCancelled);

	TWeakObjectPtr<UWxSkillCutsceneComponent> Coordinator;
	FDelegateHandle EndedHandle;
};
