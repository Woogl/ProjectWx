// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PrintSubtitle.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

/**
 * 화면 자막을 StateTree 로 띄우는 노드.
 * 자막 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 인디케이터 노드와 같은 모양이라, 퀘스트 같은 소비 도메인이 UI 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 자막은 보는 사람의 사건이라 대상 액터가 없다. 그래서 로케이터 계열 파라미터도 없다.
 */

// ── PrintSubtitle: 화면 자막 표시 ─────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_PrintSubtitleInstanceData
{
	GENERATED_BODY()

	/** 출력할 자막의 시작 행. 이후 진행(유지 시간·다음 줄)은 자막 데이터가 정한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxUI.WxSubtitleTableRow"))
	FDataTableRowHandle StartRow;

	/** (런타임) 지금 띄운 줄의 행 이름. 다음 줄을 찾을 출발점이다. */
	UPROPERTY()
	FName CurrentRowName;

	/** (런타임) 지금 띄운 줄의 유지 시간(초). 매 틱 행을 되찾지 않도록 표시할 때 받아 둔다. */
	UPROPERTY()
	float CurrentDuration = 0.f;

	/** (런타임) 이 노드가 실제로 건 자막의 핸들. 회수는 이 기록만 근거로 한다. */
	UPROPERTY()
	int32 SubtitleHandle = INDEX_NONE;

	/** (런타임) 지금 띄운 줄을 건 뒤 경과 시간(초). */
	UPROPERTY()
	float ElapsedSeconds = 0.f;
};

/**
 * 진입 시 시작 행의 줄을 걸고, 그 줄의 Duration 이 다 차면 NextRow 로 이어간다.
 * 다음 줄이 없으면 그 자리에서 자막을 걷으며 Succeeded 로 완료하고, 시간을 채우기 전에 상태를 떠나면 그때 걷는다.
 * 넘기는 사람이 있는 대화와 달리 전진도 표시 수명도 이 노드가 소유하므로 시간을 직접 센다.
 *
 * 자막을 걷는 것과 완료를 내는 것은 분리해야 한다 — 같은 상태의 다른 대기 태스크가 상태를 붙잡고 있으면(TasksCompletion=All) 완료를 내고도 한참 뒤에야 상태를 떠나므로, 회수를 ExitState 에만 맡기면 자막이 그동안 화면에 남는다.
 *
 * 행의 Duration 이 0 이하면 그 줄에서 넘어가지 않고 계속 머문다 — "자막을 띄운 채 목적지까지 이동" 같은 조합은 같은 상태에 Wait 계열 태스크를 나란히 얹어 그쪽이 완료를 내게 한다.
 * 시작 행 미지정은 기능 미사용(재사용 스텝의 옵션 파라미터)이라 경고 없이 Succeeded 로 즉시 완료한다.
 * 지정했는데 열지 못하면(행 없음·본문 빔) 낼 자막이 없으므로 경고를 남기고 Failed 다.
 * 반면 중간에 다음 행이 끊기면 경고만 남기고 거기서 끝낸다 — 데이터 오타로 트리까지 세우지는 않는다.
 *
 * 문구는 글로벌 컬렉션의 자막 뷰모델에 걸며, 그것을 보는 것은 같은 게임 인스턴스의 화면이다(v1 싱글/리슨 호스트 전제).
 */
USTRUCT(meta = (DisplayName = "Print Subtitle", Category = "Wx"))
struct FWxStateTreeTask_PrintSubtitle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PrintSubtitleInstanceData;

	FWxStateTreeTask_PrintSubtitle();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	/** 지정 행을 화면에 걸고 진행 상태를 그 줄로 맞춘다. 행이 없거나 본문이 비어 있으면 실패한다. */
	bool ShowRow(FStateTreeExecutionContext& Context, FInstanceDataType& Instance, FName RowName) const;
};
