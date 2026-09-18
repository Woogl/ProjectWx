// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_MirrorAbility.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/** Master의 커밋을 따라 동일 클래스·레벨의 어빌리티를 독립 실행한다. */
UCLASS()
class WXAI_API UWxBTTask_MirrorAbility : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UWxBTTask_MirrorAbility();
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type Result) override;
protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	FBlackboardKeySelector MirrorTarget;
	/** 어느 컨테이너든 어빌리티 AssetTags와 정확히 같은 태그가 하나라도 있으면 제외한다. 부모 태그는 매칭하지 않는다. */
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	TArray<FGameplayTagContainer> ExcludedAbilities;
private:
	bool IsExcluded(const UGameplayAbility* Ability) const;
	void ReplayAutomatic(UGameplayAbility* Ability);
	void ClearAutomaticAbilities();
	void BindMaster(UAbilitySystemComponent* ASC);
	void HandleCommitted(UGameplayAbility* Ability);
	void CleanUp();
	TWeakObjectPtr<UAbilitySystemComponent> MasterASC;
	TWeakObjectPtr<UAbilitySystemComponent> MirrorASC;
	TMap<FGameplayAbilitySpecHandle, FGameplayAbilitySpecHandle> AutomaticHandles;
	uint32 MasterGeneration = 0;
	bool bReplayingAutomatic = false;
};
