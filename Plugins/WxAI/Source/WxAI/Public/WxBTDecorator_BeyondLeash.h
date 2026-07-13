// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "WxBTDecorator_BeyondLeash.generated.h"

/**
 * BT Decorator: 폰이 배치 지점(HomeLocation)에서 LeashRadius 이상 벗어났는지(리시 이탈) 판정한다.
 *
 * 리시 검출을 퍼셉션 컴포넌트의 폴 타이머에서 걷어내 BT 로 옮긴 조건 노드다.
 * 참(이탈)이면 상위에 배치한 복귀 브랜치(UWxBTTask_ReturnHome)가 전투를 선점하도록 게이팅한다.
 * 추격 중 실시간으로 이탈을 감지해 전투를 중단시키려면 BT 에디터에서 FlowAbortMode 를 LowerPriority/Both 로 설정한다.
 *
 * HomeLocation 은 Blackboard 의 고정 키(WxBlackboardKeys::HomeLocation)에서 읽고, 이탈 반경(LeashRadius)은 디자이너가 폰별로 지정한다.
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

	/** 폰이 HomeLocation 에서 이 거리 이상 벗어나면 이탈(true)로 본다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LeashRadius = 3000.f;
};
