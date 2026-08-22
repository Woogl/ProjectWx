// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxComponentName.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SendEventToChildDevice.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SendEventToChildDeviceInstanceData
{
	GENERATED_BODY()

	/** 오너 BP 에 ChildActor 로 심긴 내장 장치(문·엘리베이터의 버튼). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedClasses = "/Script/Engine.ChildActorComponent"))
	FWxComponentName ChildDevice;

	/** 그 장치의 트리에 보낼 이벤트. 목적지 상태의 태그를 그대로 보내면 「그 상태로 가 달라」가 된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag Event;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 오너 BP 의 내장 장치 하나에 이벤트를 보내고 Succeeded 로 완료한다(엘리베이터가 자기 버튼을 잠그고 푸는 자리).
 * 어느 상태로 갈지는 그 이벤트를 듣는 대상 에셋의 전이가 정하고, 결과는 대상의 StateTag 복제로 클라에 전해진다.
 *
 * '연결 장치 발동' 의 반대 방향이다 — 그쪽이 오너가 지목한 바깥 장치들에 오너의 TriggerEvent 를 미는 것이라면, 이쪽은 오너가 품은 장치 하나에 저작된 태그를 보낸다.
 * 대상의 활성을 밖에서 직접 쓰지 않는 것이 요점이다 — 장치의 활성은 그 장치의 트리만 쓰고 밖에서는 상태를 요청하기만 하므로, 대상이 자기 사정으로 켜고 끄는 중이어도 다투지 않는다.
 * 초기 진입(StateTree 시작·세이브 복원·레이트조인)이면 보내지 않는다 — 대상도 자기 복원 경로로 같은 상태에 수렴한다.
 */
USTRUCT(meta = (DisplayName = "내장 장치에 이벤트 보내기", Category = "Wx"))
struct FWxStateTreeTask_SendEventToChildDevice : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SendEventToChildDeviceInstanceData;

	FWxStateTreeTask_SendEventToChildDevice();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
