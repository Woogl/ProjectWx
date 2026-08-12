// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayAnimation.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UAnimSequenceBase;
class USkeletalMeshComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_PlayAnimationInstanceData
{
	GENERATED_BODY()

	/** 애니메이션을 적용할 스켈레탈 메시. ST 에셋에서 Context 액터의 컴포넌트로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USkeletalMeshComponent> TargetMesh;

	/** 진입 시 재생할 애니메이션. 진입 경로를 가리지 않고 처음부터 재생한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimSequenceBase> Animation;
};

/**
 * 진입 시 지정 스켈레탈 메시에 애니메이션을 적용하고, 재생이 끝나면 Succeeded 를 반환해 상태를 완료시킨다. State 를 읽지 않는 순수 비주얼 태스크라 어떤 기믹이든 재사용한다.
 * 진입 경로(라이브 전이/초기 시작/복원/레이트조인)를 가리지 않고 처음부터 재생한다 — 복원 직후에도 그 연출이 한 번 다시 보인다('Component Move' 와 동일한 방침).
 * 재생 종료를 감지하려고 틱한다 — 싱글노드 인스턴스가 멈추면 완료로 본다.
 */
USTRUCT(meta = (DisplayName = "Play Animation", Category = "Wx"))
struct FWxStateTreeTask_PlayAnimation : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayAnimationInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
