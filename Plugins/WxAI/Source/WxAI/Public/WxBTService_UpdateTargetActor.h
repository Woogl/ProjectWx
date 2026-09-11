// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WxBTService_UpdateTargetActor.generated.h"

class AActor;
class UAIPerceptionComponent;

/**
 * BT Service: 퍼셉션이 감지한 액터 중 하나를 Blackboard 의 TargetActor 에 기록한다.
 *
 * 타겟은 죽거나 사라지거나 어그로 비허용으로 바뀌면 갈린다. 시야에서 벗어나도 유지하며, 리시 복귀는 감지 기록을 지워 해제한다.
 * 트리 어디서든 타겟이 필요하므로 루트에 달아 항상 돌린다.
 */
UCLASS()
class WXAI_API UWxBTService_UpdateTargetActor : public UBTService
{
	GENERATED_BODY()

public:
	UWxBTService_UpdateTargetActor();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	/** 세 센스 모두 적대만 등록하므로 피아는 다시 가르지 않고, 자기 폰·시체·어그로 비허용 대상을 걸러 첫 후보를 고른다. */
	AActor* FindPerceivedTarget(const UAIPerceptionComponent& Perception, const AActor* SelfActor) const;

	bool IsActorDead(AActor* Actor) const;
	bool CanBeAggroTarget(AActor* Actor) const;
};
