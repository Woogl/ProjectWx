// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SpawnActor.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AActor;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SpawnActorInstanceData
{
	GENERATED_BODY()

	/** 스폰할 액터 클래스. ST 에셋에서 직접 지정한다(레이저 벽 BP 등 클래스 레벨 에셋). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<AActor> ActorClass;

	/** 오너 액터 로컬 공간 기준의 스폰 트랜스폼. 오너 월드 트랜스폼에 합성해(위치·회전·스케일) 그 자리에 스폰한다. 기본 Identity 면 오너 트랜스폼 그대로에 스폰한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FTransform LocalSpawnTransform;

	/** 스폰 간격(초). 양수면 그 간격마다 반복 스폰한다. 0 이면 진입 직후 1회만 스폰하고 반복하지 않는다(일회성). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Interval = 2.f;

	/** 스폰체 수명(초). 0 이하면 무한(직접 파괴/이탈 정리에 맡김). 양수면 스폰 시 SetLifeSpan 으로 자동 파괴된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Lifetime = 0.f;

	/** 상태를 떠날 때 추적 중인 스폰체를 전부 파괴할지. true(기본)면 이탈 시 즉시 청소, false 면 남겨 각자 Lifetime 으로 끝까지 살다 자동 파괴된다(예: 트랩을 꺼도 떠 있던 벽은 마저 지나가게). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bDestroyOnExit = true;

	/** 스폰 시 충돌 처리 방식(FActorSpawnParameters 의 SpawnCollisionHandlingOverride 로 전달). 기본은 위치 보정 없이 항상 스폰. 겹침을 피하거나 보정하려면 디자이너가 바꾼다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	/** (런타임) 살아있는 스폰체 목록. 후속 이동 노드가 이 프로퍼티를 바인딩 소스로 읽어 이동시킬 수 있다. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** (런타임) 마지막 스폰 이후 누적 시간. Interval 도달 시 1회 스폰하고 차감한다. */
	UPROPERTY()
	float TimeSinceLastSpawn = 0.f;
};

/**
 * 매 틱 권위 측에서 LocalSpawnTransform 을 오너 월드 트랜스폼에 합성한 자리에 ActorClass 를 Interval 마다 1회 스폰하고 살아있는 목록(SpawnedActors)을 유지한다(Interval 0 이면 진입 직후 1회만 스폰하는 일회성). State 를 읽지 않아 어떤 기믹이든 주기 스폰에 재사용한다(예: LaserCorridor 의 레이저 벽).
 * 스폰 위치·회전·크기는 LocalSpawnTransform×오너 트랜스폼이 그대로 정하고(스폰체 크기는 로컬 스케일×오너 스케일), 수명은 Lifetime 으로 받아 양수면 SetLifeSpan 으로 자동 파괴한다. 스폰 충돌 처리는 SpawnCollisionHandlingOverride 로 디자이너가 정한다. 후속 이동이 필요하면 이동 노드가 SpawnedActors 를 바인딩해 구동하므로, 에셋에서 이 노드를 그 앞에 둔다.
 * 스폰은 서버 권위 사건이라 권위 측에서만 일어나고(클라는 복제 추종), 스폰체는 Transient 라 복원할 포즈가 없어 초기 진입·라이브 구분 없이 진입 즉시 스폰을 재개한다.
 * 완료 전이가 없는 머무는 태스크라 항상 Running 을 유지하며, 상태를 떠날 때 ExitState 가 bDestroyOnExit 면 남은 스폰체를 전부 파괴한다(끄면 각자 Lifetime 으로 자동 파괴되게 남긴다).
 * 완료 판정 참여 여부는 얹히는 상태가 정한다 — 대기 태스크와 같은 상태에 두면 판정에서 빼야 그 상태가 완료될 수 있고(완료를 내지 않으므로), 이 태스크만 있는 상태라면 판정 태스크가 0개가 되지 않게 남겨 둔다(형제의 완료 비트를 물려받는 엔진 동작).
 */
USTRUCT(meta = (DisplayName = "Spawn Actor", Category = "Wx"))
struct FWxStateTreeTask_SpawnActor : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SpawnActorInstanceData;

	FWxStateTreeTask_SpawnActor();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
