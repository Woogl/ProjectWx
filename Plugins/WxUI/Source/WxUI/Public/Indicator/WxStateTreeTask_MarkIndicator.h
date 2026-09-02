// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_MarkIndicator.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AWxIndicator;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

/**
 * 인디케이터 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 퀘스트 같은 소비 도메인이 UI 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 레벨 액터 지정은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다(순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커·WP·PIE 해석이 엔진에 내장).
 */

USTRUCT()
struct FWxStateTreeTask_MarkIndicatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	FUniversalObjectLocator Target;

	/** 띄울 인디케이터. 아이콘 등 표시 내용은 전부 이 클래스의 위젯이 들고 있다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<AWxIndicator> IndicatorClass;

	/** 대상 원점에서 위로 올릴 높이(cm). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float WorldZOffset = 100.f;

	/**
	 * 대상이 언로드돼 해석되지 않는 동안 가리킬 월드 좌표.
	 * 위 로케이터를 지정하면 그 액터 위치로 자동 기록된다 — 액터를 옮긴 뒤에는 다시 지정하거나 이 값을 직접 고쳐야 한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector TargetLocation = FVector::ZeroVector;

	/** (런타임) 이 노드가 실제로 띄운 인디케이터. 걷어가는 것은 이 기록만 근거로 한다. */
	UPROPERTY()
	TWeakObjectPtr<AWxIndicator> SpawnedIndicator;
};

/**
 * 진입 시 지정 대상 위에 화면 인디케이터를 띄우고, 상태에 머무는 동안 유지하다 떠날 때 걷어간다.
 * 인디케이터는 대상에 부착돼 따라다니되 화면 밖이면 스스로 화면 가장자리에 붙어 방향을 가리킨다 — 순수 월드 마커로는 줄 수 없는 정보다.
 *
 * 대상 해석은 아직 잡지 못한 동안에만 매 틱 재시도해 월드 파티션 언로드/재로드를 따라간다.
 * 해석되지 않는 동안(언로드·파괴 모두)에는 기록해 둔 좌표를 대신 가리킨다 — 마커가 가장 필요한 때가 목표가 멀어 아직 스트리밍되지 않은 때다.
 * 빈 로케이터·미지정 인디케이터는 표시될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
 * 완료 없는 머무는 태스크라 항상 Running 이다.
 * 인디케이터는 복제되지 않으므로 이 노드를 도는 머신에만 뜬다(v1 싱글/리슨 호스트 전제).
 */
USTRUCT(meta = (DisplayName = "인디케이터 표시", Category = "Wx"))
struct FWxStateTreeTask_MarkIndicator : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_MarkIndicatorInstanceData;

	FWxStateTreeTask_MarkIndicator();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 아직 대상을 잡지 못한 인디케이터만 다시 해석해 부착한다. */
	void RefreshTarget(const FStateTreeExecutionContext& Context, FInstanceDataType& Instance) const;

#if WITH_EDITOR
	virtual void PostEditInstanceDataChangeChainProperty(const FPropertyChangedChainEvent& PropertyChangedEvent, FStateTreeDataView InstanceDataView) override;
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
