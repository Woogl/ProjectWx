// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxComponentName.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SpawnNiagara.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SpawnNiagaraInstanceData
{
	GENERATED_BODY()

	/** 붙일 컴포넌트. 트리가 붙은 액터가 가진 것 중에서 고르며, 비우면 액터 위치에 재생한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxComponentName AttachComponent;

	/** 비우면 컴포넌트 원점에 붙는다. AttachComponent 를 지정했을 때만 의미가 있다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName AttachSocketName;

	/** 부착 대상이 없으면 액터 기준. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UNiagaraSystem> Niagara;

	/**
	 * (런타임) 이 노드가 마지막으로 띄운 Niagara. 진입 시 재생 여부 판정의 단일 근거다.
	 * 루프 FX 는 계속 미완료라 유지되고, 일회성 FX 는 재생이 끝나면 완료 상태가 되거나 bAutoDestroy 로 사라져 다음 진입에 다시 스폰된다.
	 */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedComponent;
};

/**
 * 진입할 때 이 노드가 띄운 Niagara 가 재생 중이 아니면 재생하고 Succeeded 로 완료한다. State 를 읽지 않아 어떤 장치든 재사용한다.
 * 진입 경로(라이브 전이/초기 시작/복원/레이트조인)를 가리지 않고 판단 기준은 그 하나다.
 * 모든 피어(서버+클라)가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다.
 */
USTRUCT(meta = (DisplayName = "나이아가라 스폰", Category = "Wx"))
struct FWxStateTreeTask_SpawnNiagara : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SpawnNiagaraInstanceData;

	FWxStateTreeTask_SpawnNiagara();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
