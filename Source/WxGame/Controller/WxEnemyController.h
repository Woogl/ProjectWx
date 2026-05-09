// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WxEnemyController.generated.h"

class UBehaviorTree;
class UWxAIPerceptionComponent;

/**
 * 에너미 AI 컨트롤러.
 *
 * Perception/감지 → Blackboard 동기화 책임은 UWxAIPerceptionComponent 에 위임한다.
 * 본 컨트롤러는 OnPossess 시 Pawn 컨텍스트 BB 키 (SelfActor, HomeLocation) 세팅과 폰의 BehaviorTree 실행만 담당한다.
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UWxAIPerceptionComponent> WxAIPerceptionComponent;
};
