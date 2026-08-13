// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeDelegate.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_EnableInteraction.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UPrimitiveComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_EnableInteractionInstanceData
{
	GENERATED_BODY()

	/** 토글할 상호작용 영역(대상 메시). ST 에셋에서 Context 액터의 메시(예: Console)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UPrimitiveComponent> TargetMesh;

	/** 진입 시 위 메시의 상호작용 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = false;

	/** 상호작용을 켤 때 이 메시가 표시할 HUD 프롬프트. 오너 기믹의 GetInteractionPrompt 로 pull 된다. 코드 폴백이 없으므로 비우면 문구 없이 표시된다. bEnable 일 때만 의미가 있다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bEnable"))
	FText Prompt;

	/** 이 메시가 눌렸을 때 오너 기믹이 발행하는 델리게이트. 전이의 Delegate 칸에서 이것을 골라 "이 영역이 눌리면 저기로" 를 잇는다(끄는 노드의 것은 발행될 일이 없다). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FStateTreeDelegateDispatcher OnInteracted;
};

/**
 * 진입 시 지정 메시의 상호작용 활성/비활성을 bEnable 로 토글한 뒤 Succeeded 로 완료한다 — 꺼진 영역은 오너 기믹의 활성 목록에서 빠져 스캔 후보에서 탈락한다.
 * 상호작용을 켜는 상태면 그 메시의 프롬프트(Prompt)도 함께 오너 기믹에 세팅해, "이 상태가 상호작용 가능한가 + 문구는 무엇인가"를 한 자리에서 author 한다(끄는 상태는 스캔에 안 잡혀 불필요, EditCondition 으로 필드 숨김).
 * 눌리면 이 노드의 OnInteracted 가 발행되고, 그것을 받아 어느 상태로 갈지는 전적으로 에셋의 전이가 정한다 — 이 노드는 목적지를 모른다.
 * 영역이 여럿이라 갈 곳이 갈리는 상태는 전이가 각각 다른 노드의 OnInteracted 를 지목하면 갈린다 — 발행자가 영역마다 하나씩이라 비교 조건이 필요 없다.
 * 그 전이는 이 노드가 있는 상태나 그 하위 상태에 두어야 한다. 바인딩이 볼 수 있는 범위가 루트에서 전이가 달린 상태까지의 경로뿐이라, 부모 상태의 전이는 자식의 발행자를 지목하지 못한다.
 * 프롬프트는 대상 메시별로 담기므로 한 상태가 여러 영역을 켜도 서로 덮어쓰지 않는다. 끄는 상태에서는 그 영역의 세팅을 지워 다시 켤 때 이전 상태의 문구를 물려받지 않는다.
 * 포즈/이동 등과 직교하는 단일 책임 태스크. 인터랙션이 여러 개인 기믹은 영역마다 노드를 둔다. 틱하지 않으므로 비용이 없다.
 * 각 상태가 자기 인터랙션 가용 여부·프롬프트를 명시하도록 상태마다 둔다(직접 복원 시에도 일관). 프롬프트는 이 태스크가 유일한 출처라 켜는 상태마다 채워야 한다 — 비우면 그 영역은 문구 없이 표시된다. 메시가 비면 Failed.
 */
USTRUCT(meta = (DisplayName = "Enable Interaction", Category = "Wx"))
struct FWxStateTreeTask_EnableInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableInteractionInstanceData;

	FWxStateTreeTask_EnableInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
