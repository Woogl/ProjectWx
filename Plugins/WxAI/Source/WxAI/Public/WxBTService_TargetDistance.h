// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WxBTService_TargetDistance.generated.h"

/**
 * BT Service: SelfActor 와 TargetActor 사이 거리를 Blackboard 의 TargetDistance(Float 키) 에 주기적으로 기록한다.
 *
 * 거리 비교 자체는 엔진 기본 Blackboard 데코레이터(arithmetic 비교: Less/Greater 등) 가 TargetDistance 키를 읽어 처리하므로,
 * 근접/원거리 패턴 분기는 커스텀 데코레이터 없이 디자이너가 임계값만 지정해 구성할 수 있다.
 *
 * Self/Target 은 Blackboard 의 SelfActor/TargetActor 키에서 읽는다(WxBlackboardKeys 규약).
 * TargetActor 가 없으면 TargetDistance 를 비워(Clear) stale 값이 남지 않게 한다.
 * 보통 전투 서브트리의 컴포지트에 부착해, 그 서브트리가 활성인 동안 거리를 갱신한다.
 */
UCLASS()
class WXAI_API UWxBTService_TargetDistance : public UBTService
{
	GENERATED_BODY()

public:
	UWxBTService_TargetDistance();

	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 수평거리(Z 무시) 로 계산한다. 슬로프나 캡슐 높이차로 근접 판정이 어긋나는 것을 막는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	bool bUse2DDistance = true;
};
