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
	// 회피 비교 기준. INDEX_NONE = 미설정.
	int32 LastChosenChild;
};

/**
 * 자식 중 무작위 1개를 골라 실행하고, 그 결과를 그대로 부모에 반환하는 Composite.
 *
 * 후보 수집 시 각 자식의 조건 Decorator 를 평가해, 실행이 막힌 자식은 추첨에서 제외한다.
 * RandomWeight Decorator 는 조건이 아니므로 이 필터에 걸리지 않지만, 가중치가 0 인 자식은 뽑힐 수 없으므로 후보에서 제외한다.
 * 유효 후보가 하나도 없으면 아무 자식도 실행하지 않고 실패를 반환한다.
 *
 * Selector 시멘틱과 다르다 — 일단 선택된 자식이 실행 후 실패해도 다른 자식으로 폴백하지 않고 그대로 실패를 반환한다.
 *
 * 조건 실패 자식에는 인덱스와 무관하게 전부 활성화 실패를 통지한다 — 엔진 Selector 는 실행할 자식을 찾은 지점에서 탐색을 멈춰 뒤쪽 자식엔 통지하지 않지만, 무작위 추첨엔 자식 간 우선순위가 없어 앞뒤를 가를 기준이 없다.
 * 그 대가로 LowerPriority·Both 데코레이터가 붙은 자식은 뒤쪽에 있어도 관찰자로 등록되며, 그 조건이 뒤집히면 실행 중인 추첨 결과가 끊기고 재추첨이 일어날 수 있다.
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
	 * 조건을 통과한 유효 후보가 1개뿐이면 회피는 무시되고 그 자식이 다시 선택된다. 조건을 통과하지 못한 자식이 회피 완화로 되살아나는 일은 없다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	bool bAvoidRepeat = true;
};
