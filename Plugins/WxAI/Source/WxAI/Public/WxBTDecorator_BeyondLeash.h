// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "WxBTDecorator_BeyondLeash.generated.h"

struct FWxBeyondLeashMemory
{
	bool bWasBeyond;
};

/**
 * BT Decorator: 폰이 앵커(기본 HomeLocation)에서 LeashRadius 이상 벗어났는지(리시 이탈) 판정한다.
 *
 * 앵커-폰 거리는 Blackboard 키가 아니라 폰 위치에서 직접 계산하므로, 값 변화를 관찰해 재평가를 촉발할 키가 없다.
 * 그래서 관찰자(aux)로 등록된 동안 TickNode 에서 이탈 여부를 매 프레임 폴링하다가, 값이 바뀌는 순간 RequestExecution 으로 플로우 재평가를 요청한다.
 *
 * 복귀가 이미 진행 중이면 거리와 무관하게 참을 유지한다 — 완료 판정은 홈 도착을 아는 복귀 Task 가 단독으로 소유한다.
 * 이 규칙이 없으면 폰이 반경 안으로 재진입하는 순간 조건이 뒤집혀 경계에서 왕복이 난다.
 *
 * FlowAbortMode 가 None 이면 관찰자로 등록되지 않아 폴링이 없고 트리 탐색 시점에만 이탈이 걸린다.
 */
UCLASS()
class WXAI_API UWxBTDecorator_BeyondLeash : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_BeyondLeash();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	virtual uint16 GetInstanceMemorySize() const override;

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** Vector 키는 그 위치를, Actor 키는 그 액터의 위치를 쓴다. 미설정이면 이탈로 보지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FBlackboardKeySelector Anchor;

	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LeashRadius = 3000.f;
};
