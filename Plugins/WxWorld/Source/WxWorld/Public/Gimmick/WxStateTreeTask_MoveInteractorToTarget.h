// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_MoveInteractorToTarget.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AController;
class UAbilitySystemComponent;
class USceneComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_MoveInteractorToTargetInstanceData
{
	GENERATED_BODY()

	/** ST 에셋에서 Context 액터의 컴포넌트(예: 상호작용 지점)로 바인딩한다. 비우면 오너 액터 트랜스폼 기준. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> AnchorComponent;

	/** 앵커(또는 오너) 기준 목표 상대 위치. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector RelativeLocation = FVector::ZeroVector;

	/** 도착 방향으로 캐릭터 yaw 를 정렬(대상 응시)할지. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bAlignRotation = true;

	/** 앵커(또는 오너) 기준 목표 상대 회전(응시 방향). yaw 만 사용한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bAlignRotation"))
	FRotator RelativeRotation = FRotator::ZeroRotator;

	/** 목표까지 이동 시간(초). 0 이하면 즉시 스냅. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 0.5f;

	/** (런타임) 시작→목표 구간의 일정 속도(초당 거리). EnterState 에서 1회 산출. */
	UPROPERTY()
	float MoveSpeed = 0.f;

	/** (런타임) 시작→목표 yaw 의 일정 회전 속도(초당 도). EnterState 에서 1회 산출. */
	UPROPERTY()
	float TurnSpeed = 0.f;

	/**
	 * (런타임) EnterState 에서 이동 입력을 실제로 막은 컨트롤러. 해제 근거를 오너 기믹의 InteractingCharacter 가 아니라 대상 자체로 두는 이유는,
	 * 그 값이 권위 측이 언제든 갱신하는 라이브 멤버이고 캐릭터가 소멸·언포제스될 수도 있어 진입 시점의 차단 대상을 되짚을 수 없기 때문이다.
	 * 카운터는 폰이 아니라 컨트롤러에 쌓이므로, 폰이 죽어도 이 기록으로 짝을 맞춰야 리스폰 후 이동이 살아난다.
	 */
	UPROPERTY()
	TWeakObjectPtr<AController> BlockedController;

	/** (런타임) EnterState 에서 어빌리티를 실제로 막은 ASC. 캐릭터가 아니라 PlayerState 에 살 수 있어 별도로 기록한다(BlockedController 와 동일한 이유). */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> BlockedAbilitySystem;
};

/**
 * 상호작용한 플레이어 캐릭터를 앵커(또는 오너) 기준 상대 위치/방향으로 일정 시간 이동·응시시키고, 도착하면 Succeeded 로 상태를 완료시킨다.
 * 대상은 오너 기믹의 InteractingCharacter 를 직접 읽는다(바인딩 입력 없음) — 값이 이미 복제되어 모든 피어가 같은 대상을 보므로 에셋에서 배선할 것이 없다.
 * 목표 = 앵커(또는 오너) 트랜스폼 ∘ 상대오프셋 이라 모든 머신에서 동일하게 계산돼, 각 피어가 자기 캐릭터 사본을 로컬 보간해도 수렴한다(별도 복제 미러 불필요, 'Component Move' 철학). 진입 시 StopMovementImmediately 로 CMC 잔여 속도를 제거한다.
 * 이동 중에는 로컬 플레이어의 입력을 막고, ExitState 가 차단을 건 대상 자체(BlockedController/BlockedAbilitySystem 기록)로 해제해 캐릭터가 소멸·언포제스돼도 스택 카운터의 짝이 맞는다.
 * 이동은 AController::SetIgnoreMoveInput, 어빌리티+점프는 ASC 의 BlockAbilitiesWithTags(Ability.Exclusive) — 액션 어빌리티가 연출 중 서로를 막는 GAS 순정 관례 그대로이며 캐릭터 CanJumpInternal 이 AreAbilityTagsBlocked(Ability.Exclusive) 로 점프를 이미 게이트하므로 점프도 함께 막힌다.
 * 카메라(look) 입력은 별개 게이트라 유지된다.
 * 예측이 발동을 게이트하므로 소유 클라(IsLocallyControlled)에서만 걸어도 충분하다.
 * 초기 진입(StateTree 시작/복원/레이트조인)이면 이동 없이 곧바로 완료한다(발동 순간에만 동작; InteractingCharacter 는 비영속이라 복원 시 비어 있음). 대상이 없어도(비캐릭터 상호작용 등) 상태가 갇히지 않게 곧바로 완료한다.
 */
USTRUCT(meta = (DisplayName = "상호작용자 이동", Category = "Wx"))
struct FWxStateTreeTask_MoveInteractorToTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_MoveInteractorToTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
