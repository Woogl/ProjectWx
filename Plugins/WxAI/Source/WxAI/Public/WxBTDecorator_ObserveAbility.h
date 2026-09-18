// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayTagContainer.h"
#include "WxBTDecorator_ObserveAbility.generated.h"

struct FAbilityEndedData;
class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * 블랙보드 키로 지목한 액터가 태그에 맞는 어빌리티를 실행 중이면 통과한다.
 * Observer aborts가 정한 관찰 구간 동안 그 액터의 어빌리티 발동·종료와 키 변경을 받아 흐름을 다시 판정한다.
 */
UCLASS()
class WXAI_API UWxBTDecorator_ObserveAbility : public UBTDecorator
{
	GENERATED_BODY()

public:
	UWxBTDecorator_ObserveAbility();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FBlackboardKeySelector ActorToObserve;

	/** 어빌리티 에셋 태그가 이 중 하나에 속하면(부모 태그 포함) 관찰 대상이다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTagContainer AbilityTags;

private:
	EBlackboardNotificationResult HandleActorToObserveChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID);
	void HandleAbilityActivated(UGameplayAbility* Ability);
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	void BindObservedASC();
	void UnbindObservedASC();

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
};
