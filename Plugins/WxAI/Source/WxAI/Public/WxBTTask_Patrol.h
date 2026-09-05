// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "WxBTTask_Patrol.generated.h"

class UGameplayEffect;

/**
 * UBTTask_MoveTo 를 상속해 이동/도착 판정/경로 실패·중단 처리는 엔진에 맡기고, 도착(성공)했을 때만 정찰 커서를 한 칸 진행한다.
 * 이동 목표는 Blackboard 의 PatrolTargetLocation 에서 읽고, 도착 시 UWxPatrolComponent 에 커서 진행을 위임한다.
 * 그 경로는 폰이 부착된 액터(스포너 등)의 것이다.
 * 갈 지점이 더 없으면(Once 완주, 또는 지점이 하나뿐인 경로) 이동 없이 브랜치를 점유한 채 머무르므로, 같은 시퀀스에서 뒤따르는 형제 노드는 더 이상 실행되지 않는다.
 * 그 상태로 중단되면 Once 만 선 자리에 남고, 나머지 모드는 재개 시 정찰 지점으로 돌아간다.
 *
 * 이동 목표 키는 InitializeFromAsset 가 PatrolTargetLocation 으로 고정하므로 저작 대상이 아니다.
 */
UCLASS(HideCategories = (Blackboard))
class WXAI_API UWxBTTask_Patrol : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UWxBTTask_Patrol();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	/** 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 0.5f;

	/**
	 * MoveSpeedMultiplier 를 SetByCaller 로 실어 부여하고 종료 시 제거한다.
	 *
	 * WxAI 는 WxCombat 에 의존하지 않으므로, 디자이너가 BT 에디터에서 직접 지정한다 (WxEffect_MoveSpeedScale).
	 * 지정하지 않으면 감속 없이 평상시 속도로 정찰한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	TSubclassOf<UGameplayEffect> MoveSpeedEffect;

	// 아래 상태들은 노드 인스턴스(폰)별로 보관된다(bCreateNodeInstance). 폰마다 독립된 커서를 가지므로 같은 경로 공유·리스폰에 안전하다.
	int32 PatrolCursor = 0;

	/** PingPong 진행 방향(+1/-1). */
	int32 PatrolDirection = 1;

	/** 갈 지점이 더 없어 제자리에 머무는 중인지. 중단되면 Once 만 이 상태를 유지한다. */
	bool bPatrolFinished = false;

	FActiveGameplayEffectHandle MoveSpeedEffectHandle;
};
