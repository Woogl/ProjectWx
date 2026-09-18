// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_FollowMasterAbility.generated.h"

struct FAbilityEndedData;
class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * Master가 목록의 스킬을 발동하면 같은 에셋 태그의 자기 어빌리티를 발동하고, 그 어빌리티가 끝나면 완료한다. 취소로 끝나면 Failed.
 * 반응하는 동안에는 Master를 듣지 않는다 — 이후 발동이 반응을 끊거나 다시 시작하지 않는다.
 */
UCLASS()
class WXAI_API UWxBTTask_FollowMasterAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_FollowMasterAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** Master 어빌리티의 에셋 태그와 정확히 같아야 따라 한다. 부모 태그는 매칭하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Categories = "Ability"))
	FGameplayTagContainer FollowedAbilities;

private:
	void HandleMasterAbilityActivated(UGameplayAbility* Ability);
	EBTNodeResult::Type Follow(const FGameplayTagContainer& MasterAbilityTags);
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	void CleanUp();

	TWeakObjectPtr<UAbilitySystemComponent> MasterASC;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	FGameplayAbilitySpecHandle ActivatedHandle;
};
