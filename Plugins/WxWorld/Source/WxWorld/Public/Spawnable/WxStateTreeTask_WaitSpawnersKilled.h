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

	/** AllowedClasses 는 픽커 후보 제한이고, 우회 지정은 ST 컴파일 에러가 잡는다(Compile 참조). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor", AllowedClasses = "/Script/WxWorld.WxSpawner"))
	TArray<FUniversalObjectLocator> Spawners;

	/** (런타임) 대기 등록 번호. 상태를 떠날 때 자기 등록만 골라 걷어내는 데 쓴다. */
	UPROPERTY()
	int32 WaitHandle = INDEX_NONE;
};

/**
 * 지정 스포너 전원이 처치 상태(IsKilled)가 될 때까지 Running 으로 대기하다 완료 시 Succeeded 를 반환한다.
 * 처치 상태(bIsKilled)는 복제되지 않으므로 권위에서 구동되는 ST 전용이다.
 * 전원이 해석(로드)되고 처치여야 통과한다 — 미해석은 판정 불가라 강제 로드 없이 대기한다.
 *
 * 폴링하지 않는다 — 진입할 때 한 번 보고, 그 뒤로는 스포너가 처치될 때마다(MarkKilled) 오는 통보에서만 다시 본다(틱 없음).
 * 대상 해석도 그 순간에만 하므로, 진입 시점에 스포너가 언로드여도 되고 대기 중에는 아무 비용이 없다.
 * 진입 시 1회 평가가 필요한 것은 이미 전원 처치인 채로 들어오는 조립(퀘스트 재수주 등) 때문이다 — 그땐 새로 올 통보가 없다.
 *
 * 대상은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커와 WP 런타임 셀·PIE 픽스업 해석이 엔진에 내장돼 있어 레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다.
 * 배열인 것은 한 전투의 스포너가 여럿일 수 있어서다 — 단일 UOL 멤버도 같은 픽커가 뜬다.
 */
USTRUCT(meta = (DisplayName = "스포너 처치 대기", Category = "Wx"))
struct FWxStateTreeTask_WaitSpawnersKilled : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitSpawnersKilledInstanceData;

	FWxStateTreeTask_WaitSpawnersKilled();

	/** 스포너가 처치된 순간 AWxSpawner::MarkKilled 가 부른다. 기다리던 노드들이 자기 지정 전원이 처치됐는지 다시 본다. */
	static void NotifySpawnerKilled();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual EDataValidationResult Compile(UE::StateTree::ICompileNodeContext& CompileContext) override;
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
