// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "WxBTTask_ReturnHome.generated.h"

/**
 * BT Task: 추격을 포기하고 배치 지점(HomeLocation)으로 복귀한다.
 *
 * UWxBTDecorator_BeyondLeash 로 게이팅한 리시 복귀 브랜치의 실행 노드다.
 * UBTTask_MoveTo 를 상속해 이동/도착/경로 실패는 엔진에 맡기고, 진입 시 퍼셉션에 "타겟 억제(disengage)"를 지시한다.
 * 억제는 현재 타겟/마지막 인지 위치를 비우고 인식(State.InCombat)을 끄며 회전 모드를 원복하고, 복귀 중 재감지를 막는다(리시 밖에선 재-어그로하지 않는 기존 거동 보존).
 * 홈 도착·중단 등 어떤 종료 경로에서도 억제를 해제해, 리시 안으로 돌아오면 다시 정상 감지·전투가 가능하게 한다.
 */
UCLASS()
class WXAI_API UWxBTTask_ReturnHome : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UWxBTTask_ReturnHome();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	virtual FString GetStaticDescription() const override;

private:
	/** 복귀 이동 속도 배율. 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 1.f;

	/** ExecuteTask 에서 낮추기 전의 MaxWalkSpeed. OnTaskFinished 에서 이 값으로 복원한다. 폰별 보관(bCreateNodeInstance). */
	float CachedMaxWalkSpeed = 0.f;
};
