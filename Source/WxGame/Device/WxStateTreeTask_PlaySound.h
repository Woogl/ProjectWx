// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlaySound.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class USoundBase;


USTRUCT()
struct FWxStateTreeTask_PlaySoundInstanceData
{
	GENERATED_BODY()

	/** 액터 위치에서 재생한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USoundBase> Sound;
};

/**
 * 라이브 발동에서만 1회 재생하는 원샷이라 장치 트리의 시작·상태 복원에서는 재생하지 않는다.
 * 재생 핸들을 남기지 않아 상태를 떠나도 멈추지 않으므로 루프 사운드를 넣지 않는다.
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
