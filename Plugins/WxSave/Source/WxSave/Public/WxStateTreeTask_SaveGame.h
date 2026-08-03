// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SaveGame.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class USceneComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SaveGameInstanceData
{
	GENERATED_BODY()

	/** 재개 지점으로 저장할 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트(체크포인트의 SceneRoot 등)로 바인딩한다. 비우면 저장 시점 플레이어 위치가 재개 지점이 된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> ResumePoint;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 라이브 상태를 활성 슬롯에 플러시·기록하고 Succeeded 로 완료한다(체크포인트 오토세이브).
 * 슬롯을 지정하지 않으므로 언제나 활성 슬롯에 그대로 덮어쓴다 — 명명 슬롯 저장은 UI 몫이다.
 * 재개 지점은 ResumePoint 를 물린 ST 에셋이 정한다: 물리면 그 컴포넌트 자리로 확정돼 플레이어가 어디에 서서 상호작용했든 같은 자리에서 재개하고(체크포인트), 비우면 저장 시점 플레이어 위치가 그대로 재개 지점이 된다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)에는 저장하지 않는다 — 막 로드한 세이브를 로드 직후 되쓰지 않게 한다.
 * 저장 파일은 서버가 소유하므로 클라 진입은 노옵이다. 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Save Game", Category = "Wx"))
struct FWxStateTreeTask_SaveGame : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SaveGameInstanceData;

	FWxStateTreeTask_SaveGame();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
