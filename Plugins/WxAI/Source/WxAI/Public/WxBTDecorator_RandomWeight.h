// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "WxBTDecorator_RandomWeight.generated.h"

/**
 * BT Decorator: UWxBTComposite_RandomChoice 의 자식 추첨 가중치를 운반한다.
 *
 * 일반 Decorator 와 달리 조건 평가용이 아니다 — CalculateRawConditionValue 가 항상 true 를 반환해 자식 실행을 절대 막지 않으며, 오직 Weight 값을 부모 RandomChoice 에 전달하는 데이터 운반 역할만 한다.
 * RandomChoice 핸들러가 각 자식의 Decorator 목록에서 이 클래스를 찾아 누적 가중치 룰렛에 사용한다.
 *
 * 이 Decorator 가 없는 자식은 가중치 1.0 으로 취급되며, Weight 가 0 이면 그 자식은 사실상 추첨에서 제외된다.
 */
UCLASS()
class WXAI_API UWxBTDecorator_RandomWeight : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_RandomWeight();

	virtual FString GetStaticDescription() const override;

	float GetWeight() const;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** 자식 추첨 가중치. 높을수록 자주 선택된다. 0 이면 추첨에서 제외된다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 1.0f;
};
