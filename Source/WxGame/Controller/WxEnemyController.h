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
 * 에너미 AI 컨트롤러.
 *
 * Perception/감지 → Blackboard 동기화 책임은 UWxAIPerceptionComponent 에 위임한다.
 * 본 컨트롤러는 OnPossess 시 Pawn 컨텍스트 BB 키 (SelfActor, HomeLocation) 세팅과 폰의 BehaviorTree 실행을 담당하고, 폰이 죽으면 BehaviorTree 를 멈춘다.
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
	/** 빙의한 폰이 죽으면 BehaviorTree 를 멈춘다. 시체는 어빌리티도 이동도 못 하므로 계속 돌아 봐야 실패만 반복한다. */
	UFUNCTION()
	void HandlePawnDeath(AWxCharacterBase* DeadCharacter);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UWxAIPerceptionComponent> WxAIPerceptionComponent;
};
