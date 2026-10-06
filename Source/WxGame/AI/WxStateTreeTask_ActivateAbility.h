// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "WxStateTreeTask_ActivateAbility.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UAbilitySystemComponent;


USTRUCT()
struct FWxStateTreeTask_ActivateAbilityInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilityHandle;

	FDelegateHandle AbilityEndedHandle;
};

/**
 * 폰이 가진 어빌리티 중 AbilityTag 를 에셋 태그로 가진 것을 발동하고, 그 어빌리티가 끝날 때까지 머문다.
 * 정상 종료면 Succeeded, 발동 실패나 캔슬이면 Failed 로 완료한다.
 * 상태를 떠날 때 어빌리티가 아직 돌고 있으면 취소한다. 취소를 거부하는 어빌리티(CanBeCanceled가 false)는 혼자 계속 돈다.
 */
USTRUCT(meta = (DisplayName = "Activate Ability", Category = "Wx"))
struct FWxStateTreeTask_ActivateAbility : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ActivateAbilityInstanceData;

	FWxStateTreeTask_ActivateAbility();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
