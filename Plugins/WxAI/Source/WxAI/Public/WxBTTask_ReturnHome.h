// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "WxBTTask_ReturnHome.generated.h"

/**
 * UWxBTDecorator_BeyondLeash 로 게이팅한 리시 복귀 브랜치의 실행 노드다.
 * UBTTask_MoveTo 를 상속해 이동/도착/경로 실패는 엔진에 맡기고, 진입 시 퍼셉션에 "타겟 억제(disengage)"를 지시한다.
 * 억제로 복귀 중 재감지를 막아, 리시 밖에선 재-어그로하지 않는 기존 거동을 보존한다.
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
};
