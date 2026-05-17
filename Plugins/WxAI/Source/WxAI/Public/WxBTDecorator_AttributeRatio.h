// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BehaviorTree/BTDecorator.h"
#include "WxBTDecorator_AttributeRatio.generated.h"

/** 어트리뷰트 비율 비교 연산자 */
UENUM()
enum class EWxAttributeRatioComparison : uint8
{
	Less			UMETA(DisplayName = "<"),
	LessOrEqual		UMETA(DisplayName = "<="),
	Equal			UMETA(DisplayName = "=="),
	GreaterOrEqual	UMETA(DisplayName = ">="),
	Greater			UMETA(DisplayName = ">"),
};

/**
 * BT Decorator: 현재 캐릭터의 어트리뷰트 비율(Attribute / MaxAttribute) 을 지정된 값과 비교한다.
 *
 * HP/MaxHP 뿐 아니라 SP/MaxSP, MP/MaxMP, DP/MaxDP 등 임의의 어트리뷰트 쌍에 사용 가능.
 * 비교 결과가 참이면 조건 통과(true), 거짓이면 조건 실패(false) 를 반환한다.
 * 실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다.
 *
 * WxAI 는 WxCombat 에 의존하지 않으므로, Attribute / MaxAttribute 는
 * 디자이너가 BT 에디터에서 직접 지정한다 (예: WxCombatAttributeSet::HP, WxCombatAttributeSet::MaxHP).
 */
UCLASS()
class WXAI_API UWxBTDecorator_AttributeRatio : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_AttributeRatio();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual FString GetStaticDescription() const override;

protected:
	/** 분자 어트리뷰트 (예: HP) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FGameplayAttribute Attribute;

	/** 분모 어트리뷰트 (예: MaxHP) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FGameplayAttribute MaxAttribute;

	/** 비교 연산자 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	EWxAttributeRatioComparison Comparison;

	/** 비교 기준 비율 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Ratio;
};
