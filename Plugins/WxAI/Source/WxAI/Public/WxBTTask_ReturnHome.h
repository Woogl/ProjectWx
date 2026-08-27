// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "WxBTTask_ReturnHome.generated.h"

/**
 * UWxBTDecorator_BeyondLeash 로 게이팅한 리시 복귀 브랜치의 실행 노드다.
 * 복귀가 언제 끝나는지는 이 Task 가 단독으로 정한다 — 게이팅 데코는 복귀가 도는 동안 참을 유지하므로 폰이 반경 안으로 재진입해도 이동을 끊지 않는다.
 * 이동이 실제로 시작된 경우에만 퍼셉션의 타겟 억제를 켜, 리시 밖에선 재-어그로하지 않는 거동을 보존한다.
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
