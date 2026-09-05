// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "WxBTTask_MirrorAbility.generated.h"

struct FAbilityEndedData;
class UAbilitySystemComponent;

/**
 * BT Task: Blackboard 로 지목한 대상이 지금 쓰고 있는 어빌리티를 같은 식별 태그로 따라 발동하고, 대상이 놓으면 함께 놓는다.
 *
 * 어빌리티는 활성 구간 동안 식별 태그 Ability.X 를 소유 태그로 발행하므로, 대상의 소유 태그가 곧 "지금 무엇을 하는가" 다.
 * 시작도 끝도 그 태그를 따라가므로 가드처럼 쥐고 있는 동안만 유지되는 것도 그대로 흉내낸다.
 *
 * 대상이 아무것도 하지 않으면 즉시 Failed 라, 얼마나 빨리 따라붙는지는 이 노드를 다시 밟는 트리의 주기가 정한다.
 * 그 주기만큼 늦게 시작하고 끝은 대상을 따라가므로 동작의 꼬리가 그만큼 잘린다 — 주기를 짧게 잡을수록 줄어든다.
 */
UCLASS()
class WXAI_API UWxBTTask_MirrorAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_MirrorAbility();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 소환물이라면 소환자를 담는 Master 가 기본값이다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FBlackboardKeySelector MirrorTarget;

	/** 대상의 활성 어빌리티 중 이 태그들에 걸리는 것만 따라한다. 부모 태그를 넣으면 자식이 모두 걸린다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTagContainer MirroredAbilities;

private:
	FGameplayTag FindMirroredTag(const UBehaviorTreeComponent& OwnerComp) const;

	UAbilitySystemComponent* FindMirrorTargetAbilitySystem(const UBehaviorTreeComponent& OwnerComp) const;

	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	void CleanUp();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	/** 이번 실행에서 따라잡은 태그. 유효한 동안에만 대상이 아직 쥐고 있는지 되묻는다. */
	FGameplayTag MirroredTag;

	FGameplayAbilitySpecHandle ActivatedHandle;

	FDelegateHandle AbilityEndedDelegateHandle;

	/**
	 * ExecuteTask 의 발동 구간 동안 true.
	 * 이 구간의 종료 통지는 FinishLatentTask 대신 ActivationResult 로 받는다.
	 */
	bool bIsActivating = false;

	/** CancelAbilityHandle 호출 중 동기 종료 콜백이 일반 완료로 처리되지 않게 보호한다. */
	bool bIsRequestingAbort = false;

	/**
	 * 발동 구간에 마지막으로 받은 종료 통지의 결과.
	 * InProgress 면 그 구간에 통지가 없었다는 뜻이다.
	 */
	EBTNodeResult::Type ActivationResult = EBTNodeResult::InProgress;
};
