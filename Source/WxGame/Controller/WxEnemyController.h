// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WxPatrolRouteProvider.h"
#include "WxEnemyController.generated.h"

enum class EWxPatrolMoveMode : uint8;

class AWxEnemyCharacter;
class UBehaviorTree;
class UWxAIPerceptionComponent;

/**
 * 에너미 AI 컨트롤러.
 *
 * Perception/감지 → Blackboard 동기화 책임은 UWxAIPerceptionComponent 에 위임한다.
 * 본 컨트롤러는 OnPossess 시 Pawn 컨텍스트 BB 키 (SelfActor, HomeLocation, 정찰) 세팅과 폰의 BehaviorTree 실행을 담당한다.
 * 정찰 경로의 좌표/커서/순회 규칙은 본 컨트롤러가 소유하고, BT 에는 IWxPatrolRouteProvider 로 진행만 노출한다.
 */
UCLASS()
class WXGAME_API AWxEnemyController : public AAIController, public IWxPatrolRouteProvider
{
	GENERATED_BODY()

public:
	AWxEnemyController();

	//~ Begin IWxPatrolRouteProvider
	virtual bool HasPatrolRoute() const override;
	virtual void AdvanceToNextPatrolPoint() override;
	//~ End IWxPatrolRouteProvider

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UWxAIPerceptionComponent> WxAIPerceptionComponent;

private:
	/** 빙의한 적의 정찰 경로를 월드 좌표 배열로 캐시하고, 첫 목표/보유 여부를 Blackboard 에 발행한다. */
	void InitializePatrol(const AWxEnemyCharacter* Enemy);

	/** 현재 커서 위치를 Blackboard 의 PatrolTargetLocation 으로 발행한다. */
	void PublishPatrolTarget();

	TArray<FVector> PatrolPoints;

	EWxPatrolMoveMode PatrolMoveMode;

	int32 PatrolCursor = 0;

	/** PingPong 진행 방향(+1/-1). */
	int32 PatrolDirection = 1;
};
