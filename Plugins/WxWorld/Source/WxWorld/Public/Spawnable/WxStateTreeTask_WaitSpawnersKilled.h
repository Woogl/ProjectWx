// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_WaitSpawnersKilled.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_WaitSpawnersKilledInstanceData
{
	GENERATED_BODY()

	/** 처치를 판정할 배치 스포너 지정. 전원이 로드되고 처치여야 완료된다. 픽커는 모든 액터를 나열하고, 스포너가 아닌 액터를 고르면 ST 컴파일에서 에러가 난다(Compile 참조). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	TArray<FUniversalObjectLocator> Spawners;
};

/**
 * 지정 스포너 전원이 처치 상태(IsKilled)가 될 때까지 Running 으로 대기하다 완료 시 Succeeded 를 반환한다.
 * 처치 상태(bIsKilled)는 복제되지 않으므로 권위에서 구동되는 ST 전용이다.
 * 전원이 해석(로드)되고 처치여야 통과한다 — 미해석은 판정 불가라 강제 로드 없이 대기한다.
 * 배열이 비었거나 빈 로케이터 항목이 있으면 잘못된 조립이므로 진입 시 경고를 남기고 계속 대기한다(침묵 완료 방지).
 * 해석은 캐시 없이 매 틱 수행한다 — 경로 조회 기반이라 소수 항목에선 비용이 무시되고 WP 언로드/재로드를 자연 처리한다.
 * 같은 상태의 'Trigger Spawners By Locator' 가 스폰한 대상을 그대로 판정하는 짝으로 쓴다.
 *
 * 대상은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커와 WP 런타임 셀·PIE 픽스업 해석이 엔진에 내장돼 있어 레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다.
 * 하나만 쓸 자리라도 배열로 받는다 — 인스턴스 데이터의 직속 UOL 멤버는 ST 에디터가 값 위젯을 만들지 못하는 엔진(5.8) 제한에 걸리고, 배열 원소는 그렇지 않다.
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
	virtual EDataValidationResult Compile(UE::StateTree::ICompileNodeContext& CompileContext) override;
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
