// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayDialogue.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_PlayDialogueInstanceData
{
	GENERATED_BODY()

	/** 출력할 대화의 시작 노드. 이후 진행(다음 행)은 대화 데이터가 정한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle StartRow;
};

/**
 * 진입 시 로컬 플레이어(0번 컨트롤러)의 대화 세션에 지정 대사를 열고, 그 대화가 끝날 때까지 Running 으로 머물다 종료되면 Succeeded 로 완료한다(독백·무전·처치 후 대사).
 * 대사를 트리가 소유하므로(대상 액터의 대화 정의가 아니라) 같은 NPC 라도 퀘스트 단계마다 다른 대사를 낼 수 있고, 화자 없는 독백도 낼 수 있다.
 * 대화 대상을 두지 않으므로 카메라는 플레이어에 머문다 — 특정 액터를 비추는 연출이 필요해지면 별도 태스크로 분리한다.
 * 세션 부재·StartRow 미지정·행 해석 실패(행 없음·대사 빔)는 완주할 수 없는 잘못된 조립이므로 경고를 남기고 Failed.
 * 상태를 먼저 떠나도 대화를 끊지 않는다 — 읽던 대사가 사라지는 편이 더 나쁘고, 세션은 자기 데이터를 끝까지 진행한다.
 * 폴링하지 않는다 — 대화를 연 직후 세션의 일회성 종료 신호에 붙고, 대화가 닫히는 순간 완료를 통보받는다(틱 없음).
 * 0번 컨트롤러 사용은 다른 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
 */
USTRUCT(meta = (DisplayName = "대화 재생", Category = "Wx"))
struct FWxStateTreeTask_PlayDialogue : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayDialogueInstanceData;

	FWxStateTreeTask_PlayDialogue();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
