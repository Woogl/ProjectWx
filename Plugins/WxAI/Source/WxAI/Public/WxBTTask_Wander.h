// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_Wander.generated.h"

class UGameplayEffect;

/** 폰의 정면(ControlRotation)을 기준으로 시계 방향 45도 간격의 8방향. */
UENUM()
enum class EWxWanderDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackRight,
	Back,
	BackLeft,
	Left,
	ForwardLeft,
};

/**
 * 타겟을 바라본 채 이동(strafe)할지는 이 태스크가 아니라 UWxBTService_LockOn 이 회전 모드를 발행해 결정한다 — 그 서비스가 붙은 브랜치 안에서 배회하면 타겟을 보며 움직인다.
 *
 * 이동은 내비게이션이 아니라 원시 입력이므로, 방향을 고를 때 그 구간이 내비메시 위인지 직접 검증한다.
 * 내비메시가 없는 맵이나 걸어갈 거리가 0인 상태(속박·Duration 0)에서는 검증이 성립하지 않아 배회하지 않는다.
 */
UCLASS()
class WXAI_API UWxBTTask_Wander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_Wander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	/** 총 배회 지속 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Duration = 1.f;

	/**
	 * 이동 가능한 방향(폰 정면 기준 8방향, 복수 선택). 매 실행마다 선택된 방향 중 하나를 무작위로 고른다.
	 *
	 * 내비메시를 벗어나는 방향은 후보에서 빠지며, 남는 후보가 없거나 애초에 하나도 고르지 않았으면 태스크가 실패해 상위 폴백 분기로 넘어간다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Bitmask, BitmaskEnum = "/Script/WxAI.EWxWanderDirection"))
	int32 Directions = 0xFF;

	/** 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 0.3f;

	/**
	 * MoveSpeedMultiplier 를 SetByCaller 로 실어 부여하고 종료 시 제거한다.
	 *
	 * WxAI 는 WxCombat 에 의존하지 않으므로, 디자이너가 BT 에디터에서 직접 지정한다 (WxEffect_MoveSpeedScale).
	 * 지정하지 않으면 감속 없이 평상시 속도로 배회한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	TSubclassOf<UGameplayEffect> MoveSpeedEffect;

private:
	FVector MoveDirection = FVector::ForwardVector;

	float ElapsedTime = 0.f;

	FActiveGameplayEffectHandle MoveSpeedEffectHandle;
};
