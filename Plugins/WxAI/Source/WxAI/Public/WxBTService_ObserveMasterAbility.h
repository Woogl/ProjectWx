// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayTagContainer.h"
#include "WxBTService_ObserveMasterAbility.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/** 최상위 분기 수명 동안 Master ASC를 구독한다. 반응 선택과 실행은 BT에 맡긴다. */
UCLASS()
class WXAI_API UWxBTService_ObserveMasterAbility : public UBTService
{
	GENERATED_BODY()
public:
	UWxBTService_ObserveMasterAbility();
	static UWxBTService_ObserveMasterAbility* Find(const UBehaviorTreeComponent& OwnerComp, const UBTNode* Context = nullptr);
	FGameplayTag GetPendingAbility() const;
	void ConsumeAbility();
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
private:
	EBlackboardNotificationResult HandleMasterChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID);
	void BindMaster();
	void UnbindMaster();
	void HandleAbilityActivated(UGameplayAbility* Ability);
	TWeakObjectPtr<UBlackboardComponent> CachedBlackboard;
	TWeakObjectPtr<UAbilitySystemComponent> MasterASC;
	TWeakObjectPtr<UAbilitySystemComponent> SynchronizedASC;
	FDelegateHandle ActivationHandle;
	FGameplayTag PendingAbility;
};
