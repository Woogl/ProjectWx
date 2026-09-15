// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_PlaySkillCutscene.generated.h"

class UWxSkillCutsceneComponent;
class ULevelSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnCutsceneCancelled);

/**
 * 서버는 공용 컷신을 요청하고 로컬 예측 태스크는 같은 컷신의 종료를 기다린다. GameState 컴포넌트가 전원 재생을 소유한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_PlaySkillCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	/**
	 * Binding Tag "Player"는 모든 클라이언트에서 동일한 시전자 AvatarActor를 가리킨다.
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
	void HandleCutsceneEnded(AActor* Avatar, uint32 FinishedId, bool bCancelled);

	UPROPERTY()
	TObjectPtr<ULevelSequence> LevelSequence;

	TWeakObjectPtr<UWxSkillCutsceneComponent> Coordinator;
	FDelegateHandle EndedHandle;
	uint32 SessionId = 0;

	float GlobalTimeDilation = 1.f;

};
