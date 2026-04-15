// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxEnemyCharacter.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "WxBossCharacter.generated.h"

/**
 * 보스 캐릭터.
 * Blackboard의 TargetActor 키를 관찰하여:
 *  - 타겟이 설정되면 글로벌 Shell 뷰모델에 자신의 ASC를 세팅
 *  - 타겟이 해제되거나 EndPlay 시 Shell 뷰모델 내부 상태를 해제
 */
UCLASS(Abstract)
class WXGAME_API AWxBossCharacter : public AWxEnemyCharacter
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	EBlackboardNotificationResult HandleBlackboardValueChanged(const UBlackboardComponent& BlackboardComp, FBlackboard::FKey ChangedKeyID);

	void ActivateBossAbilitySystemViewModel();
	void DeactivateBossAbilitySystemViewModel();
};
