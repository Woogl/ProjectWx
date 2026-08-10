// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxDialogueStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UWxDialogueComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

/**
 * 대화를 StateTree 에서 출력·판정하는 노드.
 * 대화 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 보상 지급 노드를 WxInventory 가, 인디케이터 노드를 WxUI 가 소유하는 것과 같은 모양이라,
 * 퀘스트 같은 소비 도메인이 대화 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 앞의 두 노드는 대화를 행으로 지목한다 — 대화 정의 컴포넌트가 쓰는 것과 같은 값이라, 관찰(대기)과 출력이 같은 어휘로 맞물린다.
 * 출력은 대화를 여는 자리라 시작 행을 받고, 관찰은 거쳐 가는 지점을 보므로 대화 안의 어느 행이든 받는다.
 *
 *  - WaitDialogueCompleted 는 플레이어가 스스로 건 대화를 관찰만 한다(수주·납품 게이트).
 *  - PlayDialogue 는 반대로 트리가 대사를 열어 연출한다(독백·무전·처치 후 대사).
 *  - EnableNpcInteraction 은 대화가 아니라 화자를 다룬다 — (Targets, bEnable) 로 지정 NPC 들에게 말을 걸 수 있는지를 토글한다.
 *
 * NPC 지목은 FUniversalObjectLocator 로 배치 액터를 직접 가리킨다 — 퀘스트·스포너·인디케이터 노드와 같은 방식이라 씬 픽커·WP·PIE 해석이 엔진에 내장돼 있다.
 * 하나만 쓸 자리라도 배열로 받는다 — 인스턴스 데이터의 직속 UOL 멤버는 ST 에디터가 값 위젯을 만들지 못하는 엔진(5.8) 제한에 걸리고, 배열 원소는 그렇지 않다.
 */

// ── WaitDialogueCompleted: 지정 대화의 완주 대기 ──────────────────────────────

USTRUCT()
struct FWxStateTreeTask_WaitDialogueCompletedInstanceData
{
	GENERATED_BODY()

	/**
	 * 완주를 판정할 대화의 노드. 대화 안의 어느 행이든 지목할 수 있다.
	 * 시작 행을 넣으면 그 대화 전체가, 중간·끝 행을 넣으면 그 대사까지 읽은 대화만 게이트를 통과한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle DialogueRow;

	/** (런타임) 이 상태에 머무는 동안 지정 대사를 목격했는가. 목격 후 대화가 끝나면 완주다. */
	UPROPERTY()
	bool bObservedDialogue = false;
};

/**
 * 로컬 플레이어(0번 컨트롤러)의 대화 세션을 관찰해, 이 상태에 머무는 동안 지정 대사를 거친 대화가 끝나면 Succeeded 로 완료한다.
 * 목격은 진행 중인 대사와 지정 행을 맞춰 보고, 완주는 그 뒤 대화창이 닫히는 것으로 본다.
 * 같은 NPC 라도 퀘스트 단계마다 다른 대사를 게이트로 삼을 수 있고, 대상 없는 대사(나레이션)도 같은 방식으로 판정된다.
 * 진입 이전의 대화는 세지 않는다 — 세션의 현재 대사만 관찰하는 엣지 감지라 기록이 필요 없고, 퀘스트 재탑재 시에도 게이트가 새 대화를 요구하게 된다(즉시 통과 없음).
 * 중간 행은 플레이어가 다음 대사로 넘기기 전까지만 현재라 폴링으로 본다 — 자동 넘김이 없어 한 대사가 여러 틱 머무는 지금은 놓칠 일이 없다.
 * 대화는 뜻을 해석하지 않으므로, "이 대화가 수주다" 같은 의미 판정은 이 태스크를 놓는 퀘스트 데이터의 몫이다.
 * 행 미지정은 완료될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
 * 0번 컨트롤러 사용은 다른 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
 */
USTRUCT(meta = (DisplayName = "Wait Dialogue Completed", Category = "Wx"))
struct FWxStateTreeTask_WaitDialogueCompleted : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitDialogueCompletedInstanceData;

	FWxStateTreeTask_WaitDialogueCompleted();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── PlayDialogue: 트리가 여는 대화 ────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_PlayDialogueInstanceData
{
	GENERATED_BODY()

	/** 출력할 대화의 시작 노드. 이후 진행(다음 행)은 대화 데이터가 정한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle StartRow;
};

/**
 * 진입 시 로컬 플레이어(0번 컨트롤러)의 대화 세션에 지정 대사를 열고, 그 대화가 끝날 때까지 Running 으로 머물다 종료되면 Succeeded 로 완료한다.
 * 대사를 트리가 소유하므로(대상 액터의 대화 정의가 아니라) 같은 NPC 라도 퀘스트 단계마다 다른 대사를 낼 수 있고, 화자 없는 독백도 낼 수 있다.
 * 대화 대상을 두지 않으므로 카메라는 플레이어에 머문다 — 특정 액터를 비추는 연출이 필요해지면 별도 태스크로 분리한다.
 * 세션 부재·StartRow 미지정·행 해석 실패(행 없음·대사 빔)는 완주할 수 없는 잘못된 조립이므로 경고를 남기고 Failed.
 * 상태를 먼저 떠나도 대화를 끊지 않는다 — 읽던 대사가 사라지는 편이 더 나쁘고, 세션은 자기 데이터를 끝까지 진행한다.
 * 종료 판정은 권위 측에서 세션 상태를 폴링한다. 0번 컨트롤러 사용과 함께 다른 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
 */
USTRUCT(meta = (DisplayName = "Play Dialogue", Category = "Wx"))
struct FWxStateTreeTask_PlayDialogue : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayDialogueInstanceData;

	FWxStateTreeTask_PlayDialogue();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── EnableNpcInteraction: 지정 NPC 의 상호작용 토글 ───────────────────────────

USTRUCT()
struct FWxStateTreeTask_EnableNpcInteractionInstanceData
{
	GENERATED_BODY()

	/** 상호작용을 토글할 NPC 배치 액터 지정. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	TArray<FUniversalObjectLocator> Targets;

	/** 진입 시 그 NPC 들의 상호작용 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = true;

	/**
	 * (런타임) 이 노드가 실제로 토글을 걸어 준 대화 컴포넌트.
	 * Targets 와 같은 인덱스로 짝을 이루며, 그 자리를 다시 해석한 컴포넌트가 기록과 다르면 그때 다시 적용한다.
	 */
	UPROPERTY()
	TArray<TWeakObjectPtr<UWxDialogueComponent>> AppliedTargets;
};

/**
 * 지정 NPC 들의 상호작용을 bEnable 로 토글하고 그 상태에 머물며 유지한다 — 잠긴 NPC 는 스캔 후보에서 빠져 프롬프트조차 뜨지 않고, 서버 활성 검증에도 걸려 대화가 열리지 않는다.
 * 상태를 떠나도 되돌리지 않는다. 잠금은 그 상태에 딸린 임시 연출이 아니라 월드에 남는 변경이며, 다시 열 시점은 퀘스트마다 다르므로 여는 상태에 이 태스크를 bEnable=true 로 한 번 더 두어 에셋이 정한다.
 * 상태 완료 판정에서는 빠진다(bConsideredForCompletion=false) — 완료를 내지 않는 태스크가 판정에 끼면 대기 태스크와 같은 상태(TasksCompletion=All)가 영영 완료되지 않는다.
 *
 * 토글은 대상 액터의 대화 컴포넌트에 건다 — NPC 액터 타입을 알지 않아도 되므로, 호스트가 어느 모듈에 있든 이 노드는 대화 모듈에 남는다.
 * 대상 해석은 매 틱 대상마다 수행하고, 해석된 컴포넌트가 그 자리의 기록과 다를 때만(첫 해석 성공·스트리밍 재로드로 액터가 새로 만들어진 경우) 토글을 적용한다.
 * 대상마다 독립적으로 적용되므로 하나가 스트리밍 아웃돼도 나머지 토글은 그대로 유지된다.
 * 토글은 대상 액터의 메시 콜리전이라 재로드 때 레벨에 저장된 값으로 되돌아가므로, 진입 1회 적용으로는 대상이 그 순간 언로드면 영영 적용되지 않고 상태 유지 중 스트리밍되면 적용분이 사라진다.
 * 미해석은 스트리밍 아웃의 정상 상황이라 조용히 대기한다. 잘못된 조립(지정 누락·대화 상대 아님)만 진입 시 1회 경고한다.
 * 값을 복제하지 않으므로 서버가 곧 클라인 싱글/리슨 호스트가 전제다(다른 크로스모듈 노드와 같은 전제).
 */
USTRUCT(meta = (DisplayName = "Enable Npc Interaction", Category = "Wx"))
struct FWxStateTreeTask_EnableNpcInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableNpcInteractionInstanceData;

	FWxStateTreeTask_EnableNpcInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
