// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "WxBTTask_ActivateAbility.generated.h"

struct FAbilityEndedData;
class UAbilitySystemComponent;

/**
 * BT Task: GAS 어빌리티 발동.
 *
 * AbilityTag에 매칭되는 어빌리티를 ASC에서 발동한다.
 * 어빌리티가 정상 종료되면 Succeeded, 발동 실패 또는 캔슬 시 Failed를 반환한다.
 *
 * 동일 어빌리티 연속 발동 회피는 부모 노드 (예: WxBTComposite_RandomChoice 의 bAvoidRepeat) 에서 처리.
 */
UCLASS()
class WXAI_API UWxBTTask_ActivateAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_ActivateAbility();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	/** 발동할 어빌리티의 태그 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

private:
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	void CleanUp();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	FGameplayAbilitySpecHandle ActivatedHandle;

	FDelegateHandle AbilityEndedDelegateHandle;

	/** ExecuteTask 의 발동 구간 동안 true. 이 구간의 종료 통지는 FinishLatentTask 대신 ActivationResult 로 받는다. */
	bool bIsActivating = false;

	/** 발동 구간 안에서 어빌리티가 끝났을 때의 결과. InProgress 면 아직 끝나지 않은 것. */
	EBTNodeResult::Type ActivationResult = EBTNodeResult::InProgress;
};
