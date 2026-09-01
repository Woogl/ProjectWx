// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceComponentName.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StructUtils/InstancedStruct.h"
#include "WxStateTreeTask_SendEvent.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AWxDevice;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

UENUM()
enum class EWxDeviceEventTarget : uint8
{
	/** 배치가 정하는 상대. 레벨에 따로 놓인 장치를 오너의 배선으로 받는다. */
	Linked,

	/** 저작이 정하는 상대. 오너 BP 에 심긴 내장 장치를 이름으로 지목한다. */
	Child,
};

USTRUCT()
struct FWxStateTreeTask_SendEventInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EWxDeviceEventTarget TargetKind = EWxDeviceEventTarget::Linked;

	/** 오너의 같은 이름 프로퍼티를 바인딩으로 받는 자리다 — 공유 ST 에셋을 여러 배치 인스턴스가 쓰므로 에셋에 박은 리터럴은 인스턴스마다 다를 수 없다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "TargetKind == EWxDeviceEventTarget::Linked", EditConditionHides))
	TArray<TObjectPtr<AWxDevice>> LinkedDevices;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "TargetKind == EWxDeviceEventTarget::Child", EditConditionHides, AllowedClasses = "/Script/Engine.ChildActorComponent"))
	FWxStateTreeComponentName ChildDevice;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag Event;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FInstancedStruct Payload;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 다른 장치의 트리에 이벤트를 보내고 Succeeded 로 완료한다.
 * 어느 상태로 갈지는 그 이벤트를 듣는 대상 에셋의 전이가 정하고, 결과는 대상의 StateTag 복제로 클라에 전해진다.
 *
 * 대상의 상태를 밖에서 직접 쓰지 않는 것이 요점이다 — 장치의 활성은 그 장치의 트리만 쓰고 밖에서는 상태를 요청하기만 하므로, 대상이 자기 사정으로 켜고 끄는 중이어도 다투지 않는다.
 * 초기 진입(StateTree 시작·레이트조인)이면 보내지 않는다 — 대상도 자기 복원 경로로 같은 상태에 수렴한다.
 */
USTRUCT(meta = (DisplayName = "이벤트 보내기", Category = "Wx"))
struct FWxStateTreeTask_SendEvent : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SendEventInstanceData;

	FWxStateTreeTask_SendEvent();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
