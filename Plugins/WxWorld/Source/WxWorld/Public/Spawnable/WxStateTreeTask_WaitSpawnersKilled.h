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

	/** UOL 픽커는 AllowedClasses 를 읽지 않아 모든 액터가 후보이므로, WxSpawner 가 아닌 지정은 ST 컴파일 에러가 잡는다(Compile 참조). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor", AllowedClasses = "/Script/WxWorld.WxSpawner"))
	TArray<FUniversalObjectLocator> Spawners;

	/** (런타임) 다른 태스크가 트리를 자주 깨워도 실제 조건 판정을 저주기로 제한한다. */
	float RemainingCheckTime = 0.f;

	UE::StateTree::FScheduledTickHandle ScheduledTickHandle;
};

/**
 * 지정 스포너 전원이 처치 상태(IsKilled)가 될 때까지 Running 으로 대기하다 완료 시 Succeeded 를 반환한다.
 * 처치 상태(bIsKilled)는 복제되지 않으므로 권위에서 구동되는 ST 전용이다.
 * 전원이 해석(로드)되고 처치여야 통과한다 — 미해석은 판정 불가라 강제 로드 없이 대기한다.
 *
 * 진입할 때 즉시 판정하고, 이후에는 저주기 Scheduled Tick 으로 지속 상태를 다시 본다.
 * 따라서 일반 처치뿐 아니라 스트리밍 인처럼 별도 처치 이벤트가 없는 상태 변화도 놓치지 않는다.
 *
 * 대상은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커와 WP 런타임 셀·PIE 픽스업 해석이 엔진에 내장돼 있어 레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다.
 */
USTRUCT(meta = (DisplayName = "스포너 처치 대기", Category = "Wx"))
struct FWxStateTreeTask_WaitSpawnersKilled : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitSpawnersKilledInstanceData;

	FWxStateTreeTask_WaitSpawnersKilled();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual EDataValidationResult Compile(UE::StateTree::ICompileNodeContext& CompileContext) override;
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	/** 지정이 없으면 완료할 근거도 없다. Context 는 로케이터 해석에 쓰는 태스크 오너. */
	static bool AreAllSpawnersKilled(const FInstanceDataType& Instance, UObject* Context, int32& OutResolvedCount);
};
