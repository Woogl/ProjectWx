// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "WxBTDecorator_BeyondLeash.generated.h"

struct FWxBeyondLeashMemory
{
	bool bWasBeyond;
};

/**
 * BT Decorator: 폰이 배치 지점(HomeLocation)에서 LeashRadius 이상 벗어났는지(리시 이탈) 판정한다.
 *
 * 참(이탈)이면 상위에 배치한 복귀 브랜치(UWxBTTask_ReturnHome)가 전투를 선점하도록 게이팅한다.
 * 홈-폰 거리는 Blackboard 키가 아니라 폰 위치에서 직접 계산하므로, 값 변화를 관찰해 재평가를 촉발할 키가 없다.
 * 그래서 관찰자(aux)로 등록된 동안 TickNode 에서 이탈 여부를 매 프레임 폴링하다가, 값이 바뀌는 순간 RequestExecution 으로 플로우 재평가를 요청한다(엔진 UBTDecorator_ConeCheck 와 같은 방식).
 *
 * 이 실시간 abort 는 FlowAbortMode 가 Lower Priority 일 때만 일어나며, 생성자에서 그 값을 기본으로 지정한다(전투 브랜치가 이 브랜치보다 하위 우선순위여야 함).
 * Self/Both 는 금지한다 — 복귀가 시작되면 폰이 곧 반경 안으로 재진입하는데, 그때 자기중단이 걸려 복귀가 경계에서 끊기고 재-어그로가 나 경계에서 왕복하게 된다.
 *
 * HomeLocation 은 Blackboard 의 고정 키(WxBlackboardKeys::HomeLocation)에서 읽는다.
 */
UCLASS()
class WXAI_API UWxBTDecorator_BeyondLeash : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_BeyondLeash();

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override;

	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LeashRadius = 3000.f;
};
