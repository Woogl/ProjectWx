// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WxEnemyController.generated.h"

class AWxCharacterBase;
class AWxEnemyCharacter;
class UBehaviorTree;
class UWxAIPerceptionComponent;

/**
 * Perception/감지 → Blackboard 동기화 책임은 UWxAIPerceptionComponent 에 위임한다.
 * 정찰은 UWxBTTask_Patrol 과 UWxPatrolComponent 가 폰별로 전담하므로, 컨트롤러는 정찰에 관여하지 않는다.
 */
UCLASS()
class WXGAME_API AWxEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AWxEnemyController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** 시체는 어빌리티가 State.Dead 에 막히고 이동은 래그돌이 막으므로, 트리를 계속 돌리면 실패할 브랜치만 매 틱 다시 고른다. */
	UFUNCTION()
	void HandlePawnDeath(AWxCharacterBase* DeadCharacter);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UWxAIPerceptionComponent> WxAIPerceptionComponent;
};
