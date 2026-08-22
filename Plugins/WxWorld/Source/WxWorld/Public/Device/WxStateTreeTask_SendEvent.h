// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxComponentName.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StructUtils/InstancedStruct.h"
#include "WxStateTreeTask_SendEvent.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SendEventInstanceData
{
	GENERATED_BODY()

	/** 오너 BP 에 ChildActor 로 심긴 내장 장치 하나를 지목한다. 비우면 오너가 아는 상대 전부 — LinkedDevices 로 지목한 장치들과, 오너를 민 장치가 대상이다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedClasses = "/Script/Engine.ChildActorComponent"))
	FWxComponentName ChildDevice;

	/** 보낼 이벤트. 목적지 상태의 태그를 그대로 보내면 「그 상태로 가 달라」가 된다. 비우면 오너의 TriggerEvent 를 보낸다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag Event;

	/** 이벤트에 실을 값. 받는 트리는 전이의 Payload Struct 로 타입을 좁히고 바인딩으로 읽는다. 비우면 태그만 간다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FInstancedStruct Payload;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 다른 장치의 트리에 이벤트를 보내고 Succeeded 로 완료한다.
 * 어느 상태로 갈지는 그 이벤트를 듣는 대상 에셋의 전이가 정하고, 결과는 대상의 StateTag 복제로 클라에 전해진다.
 *
 * 대상과 태그가 서로 독립인 두 축이라 조합이 전부 뜻이 통한다 — 둘 다 비우면 버튼·레버가 자기 LinkedDevices 를 미는 자리(눌렸다), 둘 다 채우면 엘리베이터가 자기 버튼을 잠그고 푸는 자리다.
 * 대상을 비운 갈래에 「오너를 민 장치」가 함께 들어가는 것이 되돌림을 성립시킨다 — 배선이 단방향이라 동작을 마친 장치는 자기를 민 버튼을 저작으로 가리킬 수 없다(레벨에 따로 놓인 버튼이 미는 피스톤이 그렇다).
 * 비웠을 때 오너의 값을 쓰는 것은 「인스턴스마다 다르면 액터에, 에셋이 정하면 태스크에」라는 구분이다 — 버튼은 같은 에셋을 여럿이 공유하면서 보낼 이벤트가 저마다 달라 액터 프로퍼티를 쓰고, 엘리베이터는 여러 대가 공유해도 보낼 이벤트가 늘 같아 에셋 값을 쓴다.
 *
 * 대상의 상태를 밖에서 직접 쓰지 않는 것이 요점이다 — 장치의 활성은 그 장치의 트리만 쓰고 밖에서는 상태를 요청하기만 하므로, 대상이 자기 사정으로 켜고 끄는 중이어도 다투지 않는다.
 * 초기 진입(StateTree 시작·세이브 복원·레이트조인)이면 보내지 않는다 — 대상도 자기 복원 경로로 같은 상태에 수렴한다.
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
