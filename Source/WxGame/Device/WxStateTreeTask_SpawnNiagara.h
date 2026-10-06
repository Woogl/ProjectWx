// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceComponentName.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SpawnNiagara.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;


USTRUCT()
struct FWxStateTreeTask_SpawnNiagaraInstanceData
{
	GENERATED_BODY()

	/** 비우면 액터 위치에 재생한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxStateTreeComponentName AttachComponent;

	/** 비우면 컴포넌트 원점에 붙는다. AttachComponent 를 지정했을 때만 의미가 있다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName AttachSocketName;

	/** 부착 대상이 없으면 액터 기준. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UNiagaraSystem> Niagara;

	/** 이 상태가 소유한 FX. 상태 이탈·트리 종료 때 제거하고 다음 진입에서 새로 생성한다. */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedComponent;
};

/** 진입 경로(라이브 전이/초기 시작/복원/레이트조인)를 가리지 않고 모든 피어가 각자 로컬 재생하므로 별도 멀티캐스트가 필요 없다. */
USTRUCT(meta = (DisplayName = "Spawn Niagara", Category = "Wx"))
struct FWxStateTreeTask_SpawnNiagara : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SpawnNiagaraInstanceData;

	FWxStateTreeTask_SpawnNiagara();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
