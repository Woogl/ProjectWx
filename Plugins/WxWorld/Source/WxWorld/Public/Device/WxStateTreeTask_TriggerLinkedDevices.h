// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_TriggerLinkedDevices.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_TriggerLinkedDevicesInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 오너 장치가 지목한 LinkedDevices 전부에 눌림을 통지하고 Succeeded 로 완료한다(버튼·레버가 문·엘리베이터를 미는 자리).
 * 받는 장치의 트리에는 오너의 TriggerEvent(기본 Event.Device.Triggered)가 나가고 당사자도 함께 넘어가므로, 어느 상태로 갈지는 그 에셋의 전이가 정한다.
 * 지목한 장치가 없어도 무동작으로 통과한다 — 아무것도 밀지 않는 발동 장치도 성립한다.
 *
 * 대상을 파라미터로 받지 않고 오너 장치의 배열을 읽는다 — ST 에셋 하나를 여러 배치 인스턴스가 공유하므로 배선은 액터에만 실릴 수 있다.
 * 초기 진입(StateTree 시작·세이브 복원·레이트조인)이면 통지하지 않는다 — 눌림은 발동 순간의 사건이다.
 */
USTRUCT(meta = (DisplayName = "연결 장치 발동", Category = "Wx"))
struct FWxStateTreeTask_TriggerLinkedDevices : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_TriggerLinkedDevicesInstanceData;

	FWxStateTreeTask_TriggerLinkedDevices();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
