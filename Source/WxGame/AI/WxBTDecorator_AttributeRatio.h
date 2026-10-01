// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "WxBTDecorator_AttributeRatio.generated.h"

/** Attribute / MaxAttribute 비율(예: HP / MaxHP)을 Ratio 와 비교한다. */
UCLASS()
class WXGAME_API UWxBTDecorator_AttributeRatio : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_AttributeRatio();

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayAttribute MaxAttribute;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TEnumAsByte<EArithmeticKeyOperation::Type> ArithmeticOperation;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Ratio;
};
