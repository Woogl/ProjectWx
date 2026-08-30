// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_EnablePlayerInput.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class APawn;
class APlayerController;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_EnablePlayerInputInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = true;

	/** (런타임) EnterState 에서 실제로 입력을 끈 폰. 그 사이 폰이 소멸·언포제스·교체될 수 있어 되돌릴 대상을 ExitState 에서 다시 조회하지 않는다. */
	UPROPERTY()
	TWeakObjectPtr<APawn> DisabledPawn;

	/** (런타임) EnterState 에서 위 폰의 입력을 끌 때 짝으로 넘긴 컨트롤러. EnableInput/DisableInput 이 같은 컨트롤러를 요구하므로 함께 기록한다. */
	UPROPERTY()
	TWeakObjectPtr<APlayerController> DisabledController;
};

/**
 * 진입 시 장치 상호작용 당사자의 로컬 폰 입력 전체를 bEnable 로 토글한 뒤 Succeeded 로 완료한다('Enable Interaction' 과 동형의 토글 태스크).
 * 진입 경로를 가리지 않으므로 직접 복원/레이트조인 시에도 일관되게 적용된다.
 * 끈 경우에는 그 대상(폰/컨트롤러)을 기록해 두고 ExitState 가 그 기록만 근거로 되돌린다 — 다음 상태에 Enable Player Input(true) 를 배선하지 않았거나 연출 중 장치 액터/셀이 사라져 ST 가 멈춰도 입력이 꺼진 채 남지 않는다.
 * 상호작용 당사자를 소유한 로컬 컨트롤러/폰이 없으면(예: 데디 서버·원격 플레이어 상태) 노옵한다. 상호작용 캐릭터의 컨트롤러를 직접 사용하므로 스플릿스크린에서도 당사자만 토글한다.
 */
USTRUCT(meta = (DisplayName = "플레이어 입력 켜기", Category = "Wx"))
struct FWxStateTreeTask_EnablePlayerInput : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnablePlayerInputInstanceData;

	FWxStateTreeTask_EnablePlayerInput();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
