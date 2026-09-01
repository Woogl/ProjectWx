// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_TriggerSpawners.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_TriggerSpawnersInstanceData
{
	GENERATED_BODY()

	/** AllowedClasses 는 픽커 후보 제한이고, 우회 지정은 ST 컴파일 에러가 잡는다(Compile 참조). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor", AllowedClasses = "/Script/WxWorld.WxSpawner"))
	TArray<FUniversalObjectLocator> Spawners;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 지정 스포너 전부의 Respawn() 을 호출하고 Succeeded 로 완료한다(1회성 스폰 트리거).
 * 초기 진입(StateTree 시작·레이트조인: SourceStateID 무효)이면 호출하지 않는다 — 스폰은 발동 순간에만 일어난다.
 * 미해석(스트리밍 아웃) 스포너는 강제 로드 없이 스킵한다.
 *
 * 대상은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커와 WP 런타임 셀·PIE 픽스업 해석이 엔진에 내장돼 있어 레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다.
 */
USTRUCT(meta = (DisplayName = "스포너 발동", Category = "Wx"))
struct FWxStateTreeTask_TriggerSpawners : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_TriggerSpawnersInstanceData;

	FWxStateTreeTask_TriggerSpawners();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual EDataValidationResult Compile(UE::StateTree::ICompileNodeContext& CompileContext) override;
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
