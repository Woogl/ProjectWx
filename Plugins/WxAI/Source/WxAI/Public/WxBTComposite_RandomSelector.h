// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "WxBTComposite_RandomSelector.generated.h"

/**
 * 자식 노드 방문 순서를 랜덤화한 Selector.
 *
 * 시멘틱은 기본 Selector 와 동일 — 자식 중 하나가 성공하면 성공, 모두 실패하면 실패.
 * 단, 평가 순서가 매 진입마다 랜덤 셔플되며 이미 실패한 자식은 같은 진입 안에선 다시 선택되지 않는다.
 * 균등 분포. 가중치가 필요해지면 자식 ChildEntry 마다 Weight UPROPERTY 추가하는 방향으로 확장.
 *
 * bAvoidRepeat 가 켜져 있으면 직전 진입에서 성공한 자식을 다음 진입의 첫 후보에서 제외한다.
 * 다른 자식이 모두 실패하면 폴백으로 선택될 수 있고, 자식이 1개뿐이면 회피는 무시된다.
 *
 * 자식 수 상한은 32 (비트마스크 폭). 그 이상은 동작은 하지만 33번째 이후 자식이 절대 선택되지 않는다.
 */
UCLASS()
class WXAI_API UWxBTComposite_RandomSelector : public UBTComposite_Selector
{
	GENERATED_BODY()

public:
	UWxBTComposite_RandomSelector(const FObjectInitializer& ObjectInitializer);

	virtual uint16 GetInstanceMemorySize() const override;

	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	virtual FString GetStaticDescription() const override;

protected:
	/**
	 * true 이면 직전 진입에서 성공한 자식 인덱스를 본 진입의 첫 선택 후보에서 제외한다.
	 * 다른 후보가 모두 실패하면 마지막 폴백으로 선택될 수 있다. 자식이 1개뿐이면 항상 그 자식 선택.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	bool bAvoidRepeat = true;

	virtual int32 GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const override;
};

/**
 * TriedMask: 이번 진입 안에서 실패해서 다시 시도하지 않을 자식 인덱스 비트마스크.
 * LastSucceededChild: 직전 진입에서 성공한 자식 인덱스. 회피 비교 기준. INDEX_NONE = 미설정.
 */
struct FWxBTRandomSelectorMemory
{
	uint32 TriedMask;
	int32 LastSucceededChild;
};
