// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxActorTarget.h"
#include "WxQuestStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UWxQuestStateTree;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

/**
 * 퀘스트 StateTree(UWxQuestComponent 러너)가 사용하는 노드 모음.
 * 컨텍스트 오너는 러너를 소유한 GameState 이며, 각 노드는 오너에서 UWxQuestComponent 를 찾아 저널 갱신·다음 퀘스트 시작을 위임한다.
 * 러너가 권위에만 존재하므로 기믹 노드와 달리 권위 게이트가 없고, 세이브 미도입이라 초기 진입 스킵 게이트도 없다 — 첫 선택이 곧 정상 실행 경로다(기믹 게이트를 복사하지 말 것).
 * 레벨 액터 지정은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다(스포너 노드와 동일 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커·WP·PIE 해석이 엔진에 내장).
 * 해석은 SyncFind(강제 로드 없음)를 캐시 없이 매 틱 수행한다 — 경로 조회라 소수 대상에선 비용이 무시되고 WP 언로드/재로드를 자연 처리한다.
 *
 *  - SetQuestTitle 은 (QuestTitle) 로 진입 시 저널을 그 제목으로 등록하고 즉시 완료한다. 퀘스트당 한 번, 진행이 시작되는 상태에 둔다.
 *  - SetQuestObjective 는 (ObjectiveText) 로 진입 시 목표를 저널에 걸고, 그 상태를 떠날 때 도로 걷어간다.
 *  - WaitMoveToTarget 은 (Target, AcceptRadius) 로 플레이어 폰(0번 컨트롤러)이 대상 반경에 들어올 때까지 대기하다 도달 시 완료한다.
 *  - StartNextQuest 는 (NextQuest) 로 다음 퀘스트 시작을 컴포넌트에 예약하고 즉시 완료한다(러너 재시작은 재진입을 피해 다음 틱).
 *
 * 저널 태스크(SetQuestTitle·SetQuestObjective)는 표시용 부수효과일 뿐이라 상태 완료 판정에서 빠진다.
 * 엔진은 상태마다 자기 태스크의 완료를 따로 보므로, 판정에 끼면 자식을 둔 상태에 얹었을 때 그 상태가 곧바로 완료돼 퀘스트가 통째로 관통된다.
 *
 * 저널 정리(Unregister)는 노드가 아니다 — 트리 종료(완료·실패·교체)를 컴포넌트가 감지해 자동 정리한다.
 */

// ── SetQuestTitle: 저널 등록 ──────────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_SetQuestTitleInstanceData
{
	GENERATED_BODY()

	/** 저널·HUD 에 표시할 퀘스트 제목. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FText QuestTitle;
};

/**
 * 진입 시 오너의 퀘스트 컴포넌트의 저널을 이 제목으로 등록하고(목표는 비움) Succeeded 로 완료한다.
 * 제목은 퀘스트당 하나이므로 스텝마다 다시 걸지 말고 진행이 시작되는 상태에 한 번만 둔다.
 * 틱하지 않으므로 비용이 없다.
 *
 * 퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이다. Failed 를 돌려주긴 하지만 이 태스크는
 * bConsideredForCompletion=false 라 엔진이 그 반환 상태를 결과에 반영하지 않는다 — 트리는 그대로 진행하며, 오조립은 경고 로그로만 드러난다.
 */
USTRUCT(meta = (DisplayName = "Set Quest Title", Category = "Wx"))
struct FWxStateTreeTask_SetQuestTitle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SetQuestTitleInstanceData;

	FWxStateTreeTask_SetQuestTitle();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── SetQuestObjective: 목표 표시 ──────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_SetQuestObjectiveInstanceData
{
	GENERATED_BODY()

	/** 저널·HUD 에 표시할 목표 문구. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FText ObjectiveText;

	/** (런타임) 이 노드가 실제로 등록한 목표의 핸들. 제거는 이 기록만 근거로 한다. */
	UPROPERTY()
	int32 ObjectiveHandle = INDEX_NONE;
};

/**
 * 진입 시 저널에 목표를 하나 걸고, 상태에 머무는 동안 유지하다 떠날 때 걷어간다.
 * 목표의 수명이 곧 그 상태의 수명이라 정리 태스크가 따로 필요 없고, 부모 상태와 자식 상태가 각각 목표를 걸면 둘이 동시에 표시된다 —
 * 병렬 상태가 없는 StateTree 에서 다중 목표는 이렇게 성립한다.
 * 완료 없이 머무는 태스크라 항상 Running 이며, 상태 완료는 짝이 되는 Wait 태스크가 낸다.
 *
 * 퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이다. Failed 를 돌려주긴 하지만 이 태스크는
 * bConsideredForCompletion=false 라 엔진이 그 반환 상태를 결과에 반영하지 않는다 — 트리는 그대로 진행하며, 오조립은 경고 로그로만 드러난다.
 */
USTRUCT(meta = (DisplayName = "Set Quest Objective", Category = "Wx"))
struct FWxStateTreeTask_SetQuestObjective : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SetQuestObjectiveInstanceData;

	FWxStateTreeTask_SetQuestObjective();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── WaitMoveToTarget: 플레이어 폰의 대상 반경 도달 대기 ───────────────────────

USTRUCT()
struct FWxStateTreeTask_WaitMoveToTargetInstanceData
{
	GENERATED_BODY()

	/** 도달을 판정할 대상 배치 액터 지정. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxActorTarget Target;

	/** 도달 판정 반경(cm). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float AcceptRadius = 300.f;
};

/**
 * 플레이어 폰(0번 컨트롤러)이 지정 대상의 AcceptRadius 안에 들어올 때까지 Running 으로 대기하다 도달 시 Succeeded 로 완료한다.
 * 대상 미해석(스트리밍 아웃)·폰 부재 동안은 판정 없이 대기한다. 0번 컨트롤러 사용은 GrantReward 등 기존 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
 * 빈 로케이터는 완료될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
 */
USTRUCT(meta = (DisplayName = "Wait Move To Target", Category = "Wx"))
struct FWxStateTreeTask_WaitMoveToTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitMoveToTargetInstanceData;

	FWxStateTreeTask_WaitMoveToTarget();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── ActivateNextQuest: 다음 퀘스트 시작 예약 ─────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_ActivateNextQuestInstanceData
{
	GENERATED_BODY()

	/** 이어서 시작할 퀘스트 에셋. 비우면 아무것도 하지 않는다(체인 종점). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSoftObjectPtr<UWxQuestStateTree> NextQuest;
};

/**
 * 진입 시 다음 퀘스트 시작을 오너의 퀘스트 컴포넌트에 예약하고 즉시 Succeeded 로 완료한다.
 * 러너 실행 콜스택 안에서는 에셋 교체가 거부되므로 즉시 시작이 아니라 다음 틱 예약이다.
 * NextQuest 가 비면 아무것도 하지 않는다(체인 종점). 퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이므로 Failed.
 */
USTRUCT(meta = (DisplayName = "Activate Next Quest", Category = "Wx"))
struct FWxStateTreeTask_ActivateNextQuest : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ActivateNextQuestInstanceData;

	FWxStateTreeTask_ActivateNextQuest();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
