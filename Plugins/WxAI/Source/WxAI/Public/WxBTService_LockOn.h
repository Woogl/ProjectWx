// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WxBTService_LockOn.generated.h"

class AAIController;
class APawn;

struct FWxLockOnMemory
{
	/** 포커스와 회전 모드를 걸어 둔 폰. 비어 있으면(IsExplicitlyNull) 적용 기록이 없다는 뜻이다. */
	TWeakObjectPtr<APawn> LockedOnPawn;
};

/**
 * BT Service: 관찰하는 브랜치가 살아 있는 동안 Blackboard TargetActor 를 컨트롤러 포커스와 폰의 strafe 회전 모드에 반영한다.
 * AI 판 락온이며, 플레이어의 락온과는 별개의 구현이다 — 겨누는 대상 자체는 AWxAIController 가 UWxLockOnComponent 에 실어 두고, 이 서비스는 그 대상을 어떻게 바라볼지만 정한다.
 *
 * 포커스는 컨트롤러가, 회전 모드는 폰이 들고 있어 소유자가 갈라지기 쉬운 상태다.
 * 이 서비스가 둘을 한 쌍으로 묶어 단독으로 소유한다 — 퍼셉션은 TargetActor 발행까지만 맡으므로 두 시스템이 같은 상태를 두고 다투지 않는다.
 * Gameplay 우선순위 포커스도 이 노드만 쓴다는 전제라, 같은 우선순위를 쓰는 엔진 노드(RotateToFaceBBEntry 등)와 한 브랜치에 두지 않는다.
 *
 * 대상 소실은 Blackboard 관찰자가 아니라 틱 폴링으로 잡는다.
 * 관찰자 알림이 브랜치를 실제로 접느냐는 게이트 데코레이터의 Observer aborts 설정에 달려 있어, 그 저작 값에 따라 해제가 늦어지기 때문이다.
 * 브랜치를 접는 판정 자체는 BT 의 몫으로 남긴다.
 */
UCLASS()
class WXAI_API UWxBTService_LockOn : public UBTService
{
	GENERATED_BODY()

public:
	UWxBTService_LockOn();

	virtual uint16 GetInstanceMemorySize() const override;

	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

	/** 엔진이 여기에 노드 타입명과 틱 주기를 덧붙여 그래프에 그린다. 최상위 GetStaticDescription 을 덮으면 그 둘이 사라진다. */
	virtual FString GetStaticServiceDescription() const override;

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	/** Blackboard 타겟·현재 폰에 적용 상태를 맞춘다. 브랜치 진입과 매 틱이 같은 경로를 탄다. */
	void SyncLockOn(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const;

	/** 컨트롤러가 응시 방향을, 폰 CMC 가 그 방향으로의 회전을 담당하므로 둘은 한 쌍으로 건다. */
	void ApplyLockOn(AAIController& AIController, APawn& Pawn, AActor& Target, FWxLockOnMemory& Memory) const;

	/**
	 * 적용 기록이 있을 때만 되돌린다. 멱등이라 틱과 브랜치 이탈 양쪽에서 불러도 안전하다.
	 * 컨트롤러가 없어도 폰의 회전 모드는 되돌려야 하므로 컨트롤러를 선택 인자로 받는다.
	 */
	void ReleaseLockOn(AAIController* AIController, FWxLockOnMemory& Memory) const;
};
