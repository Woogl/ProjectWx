// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "WxBTTask_ActivateAbility.generated.h"

struct FAbilityEndedData;
class UAbilitySystemComponent;

/**
 * BT Task: GAS 어빌리티 발동.
 *
 * ActivationTag에 매칭되는 어빌리티를 ASC에서 발동한다.
 * 어빌리티가 정상 종료되면 Succeeded, 발동 실패 또는 캔슬 시 Failed를 반환한다.
 */
UCLASS()
class WXGAME_API UWxBTTask_ActivateAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_ActivateAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

protected:
	/** 발동할 어빌리티의 태그 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

private:
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	void CleanUp();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	FDelegateHandle AbilityEndedDelegateHandle;
};
