// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "WxBTDecorator_RandomWeight.generated.h"

/**
 * BT Decorator: UWxBTComposite_RandomChoice 의 자식 추첨 가중치를 운반한다.
 *
 * 일반 Decorator 와 달리 조건 평가용이 아니다 — CalculateRawConditionValue 가 항상 true 라 자식 실행을 막지 않는다.
 *
 * 이 Decorator 가 없는 자식은 가중치 1.0 으로 취급된다.
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

	/** 높을수록 자주 선택되고, 0 이면 추첨에서 제외된다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 1.0f;
};
