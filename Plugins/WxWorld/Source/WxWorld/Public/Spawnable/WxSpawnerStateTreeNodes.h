// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxSpawnerStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AWxSpawner;

/**
 * 스포너(AWxSpawner)를 다루는 StateTree 공유 노드 모음.
 * 대상 스포너는 FUniversalObjectLocator 로 배치 액터를 직접 지정한다.
 * UOL 은 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증(ValidateNoLevelActorReferences)에 걸리지 않고, 씬 픽커와 WP 런타임 셀·PIE 픽스업 해석이 엔진에 내장돼 있어 레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다.
 * 기믹 노드(WxGimmickStateTreeNodes)와 달리 컨텍스트 액터 타입을 전제하지 않는다.
 * 공유 에셋 하나를 여러 배치 인스턴스가 쓰는 기믹 ST 는 리터럴 지정이 불가능하므로 바인딩형 Trigger Spawners 를 그대로 쓴다.
 *
 *  - TriggerSpawnersByLocator 는 (Spawners) 로 라이브 전이 진입 시 권위 측에서만 지정 스포너들의 Respawn 을 호출한다(복원 시 재실행 안 함).
 *    대상 스포너는 SpawnMode=Manual 로 두어야 BeginPlay 자동 스폰·일괄 리스폰과 겹치지 않는다.
 *  - WaitSpawnersKilled 는 (Spawners) 로 지정 스포너 전원이 처치 상태가 될 때까지 대기하다 완료한다.
 *    처치 상태(bIsKilled)는 복제되지 않으므로 권위에서 구동되는 ST 전용이다.
 *
 * 해석은 SyncFind(강제 로드 없음)라 스트리밍 아웃된 스포너는 미해석으로 남는다 — Trigger 는 스킵, Wait 는 대기한다.
 */

// ── TriggerSpawnersByLocator: 라이브 진입 시 권위 측에서 지정 스포너 트리거 ───

USTRUCT()
struct FWxStateTreeTask_TriggerSpawnersByLocatorInstanceData
{
	GENERATED_BODY()

	/** 라이브 진입 시 Respawn() 을 호출할 배치 스포너 지정. 픽커는 WxSpawner 만 나열한다(AllowedClasses — WxActorLocatorEditor 가 해석). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor", AllowedClasses = "/Script/WxWorld.WxSpawner"))
	TArray<FUniversalObjectLocator> Spawners;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 지정 스포너 전부의 Respawn() 을 호출하고 Succeeded 로 완료한다(1회성 스폰 트리거).
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효) 또는 복원 마커(StateTree.Restore)면 호출하지 않는다 — 스폰은 발동 순간에만 일어난다.
 * 미해석(스트리밍 아웃) 스포너는 강제 로드 없이 스킵한다.
 * 배열이 비었거나 전부 미해석이면 조립·배치 실수일 수 있어 경고를 남긴다.
 * 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Trigger Spawners By Locator", Category = "Wx"))
struct FWxStateTreeTask_TriggerSpawnersByLocator : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_TriggerSpawnersByLocatorInstanceData;

	FWxStateTreeTask_TriggerSpawnersByLocator();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── WaitSpawnersKilled: 지정 스포너 전원 처치 대기 ───────────────────────────

USTRUCT()
struct FWxStateTreeTask_WaitSpawnersKilledInstanceData
{
	GENERATED_BODY()

	/** 처치를 판정할 배치 스포너 지정. 전원이 로드되고 처치여야 완료된다. 픽커는 WxSpawner 만 나열한다(AllowedClasses — WxActorLocatorEditor 가 해석). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor", AllowedClasses = "/Script/WxWorld.WxSpawner"))
	TArray<FUniversalObjectLocator> Spawners;
};

/**
 * 지정 스포너 전원이 처치 상태(IsKilled)가 될 때까지 Running 으로 대기하다 완료 시 Succeeded 를 반환한다.
 * 전원이 해석(로드)되고 처치여야 통과한다 — 미해석은 판정 불가라 강제 로드 없이 대기한다.
 * 배열이 비었거나 빈 로케이터 항목이 있으면 잘못된 조립이므로 진입 시 경고를 남기고 계속 대기한다(침묵 완료 방지).
 * 해석은 캐시 없이 매 틱 수행한다 — 경로 조회 기반이라 소수 항목에선 비용이 무시되고 WP 언로드/재로드를 자연 처리한다.
 * 같은 상태의 TriggerSpawnersByLocator 가 스폰한 대상을 그대로 판정하는 짝으로 쓴다.
 */
USTRUCT(meta = (DisplayName = "Wait Spawners Killed", Category = "Wx"))
struct FWxStateTreeTask_WaitSpawnersKilled : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitSpawnersKilledInstanceData;

	FWxStateTreeTask_WaitSpawnersKilled();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
