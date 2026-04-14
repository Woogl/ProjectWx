// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxEnemyCharacter.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "WxBossCharacter.generated.h"

class UWxViewModel_AbilitySystem;

/**
 * 보스 캐릭터.
 * Blackboard의 TargetActor 키를 관찰하여:
 *  - 타겟이 설정되면 자신의 어빌리티 시스템 뷰모델을 글로벌에 등록
 *  - 타겟이 해제되거나 EndPlay 시 글로벌 뷰모델을 등록 해제
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

	void RegisterBossAbilitySystemViewModel();
	void UnregisterBossAbilitySystemViewModel();

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_AbilitySystem> BossAbilitySystemViewModel;
};
