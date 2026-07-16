// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "WxBTTask_ActivatePattern.generated.h"

struct FAbilityEndedData;
class AAIController;
class APawn;
class UAbilitySystemComponent;

/**
 * BT Task: 타겟에게 접근한 뒤 패턴 어빌리티를 발동한다.
 *
 * "사정거리만큼 이동 후 발동"이 잦은 근접 패턴을 한 노드로 묶는다.
 * Blackboard 의 TargetActor 로 ApproachDistance(acceptance radius) 까지 MoveToActor 한 뒤, AbilityTag 에 매칭되는 어빌리티를 ASC 에서 발동한다.
 * 타겟이 없거나 ApproachDistance 가 0 이하면 이동을 생략하고 즉시 발동한다(제자리/자기타겟 패턴).
 * 어빌리티가 정상 종료되면 Succeeded, 이동 실패·발동 실패·캔슬 시 Failed 를 반환한다.
 *
 * 발동 전용이면 UWxBTTask_ActivateAbility 를 쓴다. 거리 판정으로 패턴 브랜치를 고르는 것은 상위 데코레이터(UWxBTService_TargetDistance + 스톡 비교)가 담당하고, 본 Task 는 고른 브랜치 안에서 접근+발동만 실행한다.
 * 동일 어빌리티 연속 발동 회피는 부모 노드(예: UWxBTComposite_RandomChoice 의 bAvoidRepeat)에서 처리한다.
 */
UCLASS()
class WXAI_API UWxBTTask_ActivatePattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_ActivatePattern();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 발동할 어빌리티의 태그 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** 타겟까지 접근할 목표 거리(cm). MoveTo 의 acceptance radius. 0 이하이거나 타겟이 없으면 이동 없이 즉시 발동한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0"))
	float ApproachDistance = 150.f;

private:
	EBTNodeResult::Type BeginActivateAbility(APawn* Pawn);

	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	void CleanUp();

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	TWeakObjectPtr<AAIController> CachedAIController;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	FGameplayAbilitySpecHandle ActivatedHandle;

	FDelegateHandle AbilityEndedDelegateHandle;

	FAIRequestID MoveRequestID;

	bool bMovePhaseActive = false;
};
