// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SaveGame.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct FWxStateTreeTask_SaveGameInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 라이브 상태를 활성 슬롯에 플러시·기록하고 Succeeded 로 완료한다(체크포인트 오토세이브).
 * 슬롯을 지정하지 않으므로 언제나 활성 슬롯에 그대로 덮어쓴다 — 명명 슬롯 저장은 UI 몫이다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효) 또는 복원 마커가 오면 저장하지 않는다 — 막 로드한 세이브를 로드 직후 되쓰지 않게 한다.
 * 저장 파일은 서버가 소유하므로 클라 진입은 노옵이다. 틱하지 않으므로 비용이 없다.
 * 무틱 즉시완료 태스크라 상태 완료를 구동하지 않는다(bConsideredForCompletion=false; 체크포인트 Lit 같은 정지 leaf 에 놓여도 재선택 루프에 빠지지 않도록).
 */
USTRUCT(meta = (DisplayName = "Wx Save Game"))
struct FWxStateTreeTask_SaveGame : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SaveGameInstanceData;

	FWxStateTreeTask_SaveGame();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
