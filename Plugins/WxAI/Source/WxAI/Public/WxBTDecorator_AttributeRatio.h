// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "WxBTDecorator_AttributeRatio.generated.h"

/**
 * BT Decorator: 현재 캐릭터의 어트리뷰트 비율(Attribute / MaxAttribute) 을 지정된 값과 비교한다.
 *
 * 실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다.
 *
 * WxAI 는 WxCombat 에 의존하지 않으므로, Attribute / MaxAttribute 는 디자이너가 BT 에디터에서 직접 지정한다 (예: WxCombatAttributeSet::HP, WxCombatAttributeSet::MaxHP).
 */
UCLASS()
class WXAI_API UWxBTDecorator_AttributeRatio : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_AttributeRatio();

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	/** 분자 어트리뷰트 (예: HP) */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayAttribute Attribute;

	/** 분모 어트리뷰트 (예: MaxHP) */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayAttribute MaxAttribute;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TEnumAsByte<EArithmeticKeyOperation::Type> ArithmeticOperation;

	/** 비교 기준 비율 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Ratio;
};
