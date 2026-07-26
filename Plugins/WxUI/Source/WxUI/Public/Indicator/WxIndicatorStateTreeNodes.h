// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxActorTarget.h"
#include "WxIndicatorStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UWxIndicatorDescriptor;
class UWxIndicatorWidget;

/**
 * 화면 인디케이터를 StateTree 로 켜고 끄는 노드.
 * 인디케이터 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 보상 지급 노드를 WxInventory 가 소유하는 것과 같은 모양이라,
 * 퀘스트 같은 소비 도메인이 UI 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 레벨 액터 지정은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다(순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커·WP·PIE 해석이 엔진에 내장).
 * 단일 대상 필드는 FWxActorTarget 래퍼(WxCore)로 감싼다 — 직속 UOL 멤버는 ST 에디터가 값 위젯을 만들지 못하는 엔진 제한이 있다(래퍼 주석 참조).
 */

// ── MarkIndicator: 대상 위에 화면 인디케이터 표시 ──────────────────────────────

USTRUCT()
struct FWxStateTreeTask_MarkIndicatorInstanceData
{
	GENERATED_BODY()

	/** 가리킬 대상 배치 액터 지정. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxActorTarget Target;

	/** 화면에 띄울 인디케이터 위젯. 소프트 참조라 표시 시점에 로드된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSoftClassPtr<UWxIndicatorWidget> IndicatorWidgetClass;

	/** 대상 원점에서 위로 올릴 높이(cm). 액터 원점이 보통 발밑이므로 머리 위를 가리키게 한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float WorldZOffset = 150.f;

	/**
	 * (런타임) 이 노드가 실제로 등록한 인디케이터. 해제는 이 기록만 근거로 한다.
	 * 대상이 아직 언로드면 비어 있고, Tick 이 해석·등록을 재시도한다.
	 */
	UPROPERTY()
	TWeakObjectPtr<UWxIndicatorDescriptor> RegisteredIndicator;
};

/**
 * 진입 시 지정 대상 위에 화면 인디케이터를 등록하고, 상태에 머무는 동안 유지하다 떠날 때 해제한다.
 * 인디케이터는 월드가 아니라 화면에 있으므로 대상이 화면 밖이면 화면 가장자리에 붙어 방향을 가리킨다 — 마커 액터로는 줄 수 없는 정보다.
 *
 * 대상 해석은 매 틱 수행해 월드 파티션 언로드/재로드를 그대로 따라간다. 해석되면 등록하고 해석되지 않으면(스트리밍 아웃·파괴) 해제하므로,
 * 표시 수명의 단일 소유자는 이 노드다.
 * 빈 로케이터는 표시될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
 * 완료 없는 머무는 태스크라 항상 Running 이다 — 에셋에서 상태 완료 판정에서 제외해야 한다(완료는 짝이 되는 Wait 태스크가 낸다).
 * 표시는 보는 사람의 사건이라 매니저는 로컬 플레이어(0번 컨트롤러)에서 찾는다(v1 싱글/리슨 호스트 전제).
 */
USTRUCT(meta = (DisplayName = "Mark Indicator", Category = "Wx"))
struct FWxStateTreeTask_MarkIndicator : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_MarkIndicatorInstanceData;

	FWxStateTreeTask_MarkIndicator();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
