// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxSubtitleStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

/**
 * 화면 자막을 StateTree 로 띄우는 노드.
 * 자막 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 인디케이터 노드와 같은 모양이라,
 * 퀘스트 같은 소비 도메인이 UI 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 자막은 보는 사람의 사건이라 대상 액터가 없다. 그래서 로케이터 계열 파라미터도 없다.
 */

// ── PrintSubtitle: 화면 자막 표시 ─────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_PrintSubtitleInstanceData
{
	GENERATED_BODY()

	/** 화면에 띄울 자막 본문. 한 노드가 한 줄을 맡는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FText SubtitleText;

	/** 자막을 유지할 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Duration = 3.f;

	/** (런타임) 이 노드가 실제로 건 자막의 핸들. 회수는 이 기록만 근거로 한다. */
	UPROPERTY()
	int32 SubtitleHandle = INDEX_NONE;

	/** (런타임) 진입 후 경과 시간(초). */
	UPROPERTY()
	float ElapsedSeconds = 0.f;
};

/**
 * 진입 시 자막을 걸고, Duration 이 다 차면 그 자리에서 자막을 걷으며 Succeeded 로 완료한다.
 * 시간을 채우기 전에 상태를 떠나면 그때 걷는다. 표시 수명의 단일 소유자가 이 노드이므로 시간도 직접 센다.
 *
 * 자막을 걷는 것과 완료를 내는 것은 분리해야 한다 — 같은 상태의 다른 대기 태스크가 상태를 붙잡고 있으면(TasksCompletion=All)
 * 완료를 내고도 한참 뒤에야 상태를 떠나므로, 회수를 ExitState 에만 맡기면 자막이 그동안 화면에 남는다.
 *
 * Duration 이 0 이하면 완료를 내지 않고 계속 머문다 — "자막을 띄운 채 목적지까지 이동" 같은 조합은
 * 같은 상태에 Wait 계열 태스크를 나란히 얹어 그쪽이 완료를 내게 한다.
 * 빈 문구는 표시될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
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
};
