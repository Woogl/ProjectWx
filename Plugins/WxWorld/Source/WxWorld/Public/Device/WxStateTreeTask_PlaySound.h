// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlaySound.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class USoundBase;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_PlaySoundInstanceData
{
	GENERATED_BODY()

	/** 액터 위치에서 재생한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USoundBase> Sound;

	/** false(기본)면 라이브 발동에서만 1회 재생(트리거 사운드), true 면 로드/복원 시에도 재생한다(상태에 묶인 지속 사운드용). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bPlayOnRestore = false;
};

/**
 * State 를 읽지 않아 어떤 장치든 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 기본적으로 재생하지 않는다 — 발동 사운드는 발동 순간에만 울리고 복원 시엔 침묵한다.
 * bPlayOnRestore 면 복원/시작 진입에서도 재생한다 — 트리거가 아니라 상태가 켜져 있는 동안 울려야 하는 지속 사운드용.
 * 모든 피어(서버+클라)가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다.
 */
USTRUCT(meta = (DisplayName = "사운드 재생", Category = "Wx"))
struct FWxStateTreeTask_PlaySound : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlaySoundInstanceData;

	FWxStateTreeTask_PlaySound();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
