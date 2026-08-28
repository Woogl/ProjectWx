// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_ApplyGameplayEffectToInteractor.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UGameplayEffect;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

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
 * 대상은 오너 장치의 InteractingCharacter 이며, 그 ASC 를 찾아 자기 자신에게 스펙을 적용한다(레벨 1 고정, 소스 오브젝트는 오너).
 * 초기 진입(StateTree 시작/복원/레이트조인)이면 적용하지 않는다 — 회복은 발동 순간의 효과라 로드 때 다시 일어나면 안 된다.
 * 어떤 GE 를 줄지는 에셋에서 정하므로 이 노드는 전투 도메인을 알지 못한다(클래스 참조는 에셋 레벨).
 */
USTRUCT(meta = (DisplayName = "상호작용자에게 이펙트 적용", Category = "Wx"))
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
