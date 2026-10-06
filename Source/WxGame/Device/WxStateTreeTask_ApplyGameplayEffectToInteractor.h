// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_ApplyGameplayEffectToInteractor.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UGameplayEffect;


USTRUCT()
struct FWxStateTreeTask_ApplyGameplayEffectToInteractorInstanceData
{
	GENERATED_BODY()

	/** 비우면 노옵. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<UGameplayEffect> EffectClass;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 상호작용 당사자에게 GameplayEffect 를 적용하고 Succeeded 로 완료한다(체크포인트 회복 등).
 * 초기 진입(StateTree 시작/레이트조인)이면 적용하지 않는다 — 회복은 발동 순간의 효과라 다시 일어나면 안 된다.
 * 어떤 GE 를 줄지는 에셋에서 정하므로 이 노드는 전투 도메인을 알지 못한다(클래스 참조는 에셋 레벨).
 */
USTRUCT(meta = (DisplayName = "Apply Effect to Interactor", Category = "Wx"))
struct FWxStateTreeTask_ApplyGameplayEffectToInteractor : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ApplyGameplayEffectToInteractorInstanceData;

	FWxStateTreeTask_ApplyGameplayEffectToInteractor();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const override;
#endif
};
