// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SetQuestTitle.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SetQuestTitleInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FText QuestTitle;
};

/**
 * 진입 시 오너의 퀘스트 컴포넌트의 저널을 이 제목으로 등록한다(목표는 비움).
 *
 * 완료 판정에 참여하되 진입 즉시 Succeeded 로 끝나며, 상태 완료는 스텝 상태의 대기 태스크가 낸다.
 * 판정에서 빼면 이 상태에 판정 태스크가 하나도 남지 않아 형제 상태의 완료를 물려받는다(함정의 상세는 WxQuest README 의 완료 판정 절).
 *
 * 퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이므로 Failed 로 그 상태를 실패시킨다.
 */
USTRUCT(meta = (DisplayName = "퀘스트 제목 설정", Category = "Wx"))
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
