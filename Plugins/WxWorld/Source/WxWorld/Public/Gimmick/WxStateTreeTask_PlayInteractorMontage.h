// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayInteractorMontage.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UAnimMontage;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_PlayInteractorMontageInstanceData
{
	GENERATED_BODY()

	/** 재생할 몽타주. 비면 재생 없이 곧바로 완료한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimMontage> Montage;
};

/**
 * 상호작용한 플레이어 캐릭터의 메시에 몽타주를 재생하고, 재생이 끝나면 Succeeded 로 상태를 완료시킨다.
 * 대상은 오너 기믹의 InteractingCharacter 를 직접 읽는다(바인딩 입력 없음) — 'Move Interactor To Target' 과 동일.
 * 복제형 PlayAnimMontage 가 아니라 각 머신이 메시 AnimInstance 로 로컬 재생·폴링한다('Play Animation' 과 동형) — 모든 피어가 InteractingCharacter 를 복제로 알아 중복 재생이 없다.
 * 초기 진입(StateTree 시작/복원/레이트조인)이면 재생 없이 곧바로 완료한다(발동 순간에만 재생; InteractingCharacter 는 비영속이라 복원 시 비어 있음). 대상/몽타주가 없어도 상태가 갇히지 않게 곧바로 완료한다.
 * 이동 후 재생하려면 'Move Interactor To Target' 상태 다음 상태에 둔다(단일 책임 분리).
 */
USTRUCT(meta = (DisplayName = "Play Interactor Montage", Category = "Wx"))
struct FWxStateTreeTask_PlayInteractorMontage : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayInteractorMontageInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
