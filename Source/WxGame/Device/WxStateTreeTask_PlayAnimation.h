// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceComponentName.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayAnimation.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UAnimSequenceBase;
class USkeletalMeshComponent;


USTRUCT()
struct FWxStateTreeTask_PlayAnimationInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedClasses = "/Script/Engine.SkeletalMeshComponent"))
	FWxStateTreeComponentName TargetMesh;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh;
};

/**
 * 진입 경로(라이브 전이/초기 시작/레이트조인)를 가리지 않고 처음부터 재생한다 — 초기 시작 직후에도 그 연출이 한 번 다시 보인다.
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
