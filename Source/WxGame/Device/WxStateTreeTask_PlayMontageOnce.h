// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayMontageOnce.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AActor;
class UAbilitySystemComponent;
class UAnimMontage;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 3 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_PlayMontageOnceInstanceData
{
	GENERATED_BODY()

	/** 몽타주를 재생할 액터. 장치에서는 Actor.InteractingCharacter 에 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** 있으면 Target 이 이쪽을 바라본다. 장치에서는 Actor 에 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimMontage> Montage;

	/** (런타임) 권위에서 부여한 어빌리티. 스펙이 걷히면 끝난 것이다. */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilityHandle;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 Target 에게 UWxAbility_PlayMontageOnce 를 1회 부여·발동하고, 그 어빌리티가 끝나면(몽타주 종료·취소, Target 소멸) Succeeded 로 완료한다.
 * 끝을 아는 것은 부여한 권위뿐이라 권위가 아닌 피어에서는 Running 으로 머문다 — 장치 트리에서는 서버가 발행하는 다음 상태로 넘어가므로, 이 태스크를 둔 상태와 다음 상태는 서로 다른 태그 상태여야 한다.
 * 초기 진입(StateTree 시작/레이트조인: SourceStateID 무효)이면 재생하지 않고 곧바로 완료한다 — 발동 순간의 연출이다.
 */
USTRUCT(meta = (DisplayName = "몽타주 1회 재생", Category = "Wx"))
struct FWxStateTreeTask_PlayMontageOnce : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayMontageOnceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
