// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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

	/** ST 에셋에서 Context 액터의 컴포넌트로 바인딩한다. 비우면 액터 위치에 재생. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> AttachComponent;

	/** 비우면 컴포넌트 원점에 붙는다. AttachComponent 를 지정했을 때만 의미가 있다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName AttachSocketName;

	/** 소켓이 없는 메시에서 불꽃 높이 같은 것을 잡을 때 쓴다(부착 대상이 없으면 액터 기준). */
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
 * 진입할 때 이 노드가 띄운 Niagara 가 재생 중이 아니면 재생하고 Succeeded 로 완료한다. State 를 읽지 않아 어떤 기믹이든 재사용한다.
 * 진입 경로(라이브 전이/초기 시작/복원/레이트조인)를 가리지 않고 판단 기준은 그 하나다.
 * 그래서 루프 Niagara 를 지정하면 상태에 묶인 지속 FX 가 되어 배선 없이 로드·복원·스트리밍 인에서 알아서 살아나고(예: 체크포인트 모닥불), 진입이 반복돼도 이미터가 겹쳐 쌓이지 않는다.
 * 반대로 일회성 FX 는 재생이 끝난 뒤 다시 진입하면 다시 터지므로, 복원 시 침묵해야 하는 순간 연출에는 맞지 않는다.
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
