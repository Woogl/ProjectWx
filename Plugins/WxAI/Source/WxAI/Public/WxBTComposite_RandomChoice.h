// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "WxBTComposite_RandomChoice.generated.h"

/**
 * 베이스(UBTCompositeNode)가 노드 메모리 앞쪽을 FBTCompositeMemory(CurrentChild/OverrideChild)로 사용하므로, 자체 상태는 반드시 그 뒤에 배치해 겹치지 않게 한다. (엔진의 FBTParallelMemory 와 동일 패턴)
 */
struct FWxBTRandomChoiceMemory : public FBTCompositeMemory
{
	// 직전 진입에서 선택된 자식 인덱스. 회피 비교 기준. INDEX_NONE = 미설정.
	int32 LastChosenChild;
};

/**
 * 자식 중 무작위 1개를 골라 실행하고, 그 결과를 그대로 부모에 반환하는 Composite.
 *
 * 후보 수집 시 각 자식의 조건 Decorator(CompareAttributeRatio 등)를 평가해, 실행이 막힌 자식은 추첨에서 제외한다. 즉 "현재 유효한 후보들 중에서만" 가중 추첨한다. RandomWeight Decorator 는 조건이 아니므로 이 필터에 걸리지 않는다.
 *
 * Selector 시멘틱과 다르다 — 일단 선택된 자식이 실행 후 실패해도 다른 자식으로 폴백하지 않고 그대로 실패를 반환한다. (조건 필터는 실행 전 후보 선정에만 적용되고, 이 폴백 없음 규칙은 런타임 실패에 적용된다.)
 * 한 진입에서 실행되는 자식은 정확히 1개. 보스/적 공격 패턴 분기처럼 "여러 후보 중 하나만 실행" 시멘틱에 적합.
 *
 * bAvoidRepeat 가 켜져 있으면 직전 진입에서 선택된 자식을 본 진입의 후보에서 제외한다.
 * 자식이 1개뿐이면 회피는 무시되고 항상 그 자식이 선택된다.
 *
 * 베이스 클래스로 UBTComposite_Selector 를 사용하는 것은 시멘틱 일치가 아니라 BT 시스템 호환성 때문이며, 실제 동작은 GetNextChildHandler 오버라이드 한 곳에서 결정된다.
 */
UCLASS()
class WXAI_API UWxBTComposite_RandomChoice : public UBTComposite_Selector
{
	GENERATED_BODY()

public:
	UWxBTComposite_RandomChoice(const FObjectInitializer& ObjectInitializer);

	virtual uint16 GetInstanceMemorySize() const override;

	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	virtual FString GetStaticDescription() const override;

	virtual int32 GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const override;

protected:
	/**
	 * true 이면 직전 진입에서 선택된 자식을 본 진입의 후보에서 제외한다.
	 * 자식이 1개뿐이면 회피는 무시되고 항상 그 자식이 선택된다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	bool bAvoidRepeat = true;
};
