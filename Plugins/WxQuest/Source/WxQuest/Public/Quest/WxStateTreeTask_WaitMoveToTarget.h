// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_WaitMoveToTarget.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_WaitMoveToTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	FUniversalObjectLocator Target;

	/** 도달 판정 반경(cm). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float AcceptRadius = 300.f;
};

/**
 * 플레이어 폰(0번 컨트롤러)이 지정 대상의 AcceptRadius 안에 들어올 때까지 Running 으로 대기하다 도달 시 Succeeded 로 완료한다.
 * 대상 미해석(스트리밍 아웃)이면 판정하지 않고, 폰 부재 동안도 마찬가지로 대기한다.
 * 0번 컨트롤러 사용은 GiveRewards 등 기존 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
 * 빈 로케이터는 완료될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
 *
 * 대상은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커·WP·PIE 해석이 엔진에 내장돼 있다.
 * 해석은 SyncFind(강제 로드 없음)를 캐시 없이 매 틱 수행한다 — 경로 조회라 비용이 무시되고 WP 언로드/재로드를 자연 처리한다.
 */
USTRUCT(meta = (DisplayName = "목표 지점 도달 대기", Category = "Wx"))
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

private:
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FString GetTargetDisplayName(const FUniversalObjectLocator& Locator) const;
#endif
};
