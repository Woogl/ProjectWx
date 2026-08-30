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
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

private:
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	void CleanUp();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

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
