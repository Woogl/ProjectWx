// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "WxBTTask_ReturnHome.generated.h"

/**
 * UWxBTDecorator_BeyondLeash 로 게이팅한 리시 복귀 브랜치의 실행 노드다.
 * 복귀가 언제 끝나는지는 이 Task 가 단독으로 정한다 — 게이팅 데코는 복귀가 도는 동안 참을 유지하므로 폰이 반경 안으로 재진입해도 이동을 끊지 않는다.
 * 이동이 실제로 시작되면 현재 타겟의 Perception 기록과 적용 상태를 잊는다.
 * 이후 새 자극은 억제하지 않으므로 정상 감지 경로를 통해 다시 타겟을 획득할 수 있다.
 */
UCLASS()
class WXAI_API UWxBTTask_ReturnHome : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UWxBTTask_ReturnHome();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
