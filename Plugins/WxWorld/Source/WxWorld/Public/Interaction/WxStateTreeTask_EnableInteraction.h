// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeDelegate.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_EnableInteraction.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AActor;
class UPrimitiveComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_EnableInteractionInstanceData
{
	GENERATED_BODY()

	/** 상태에 따라 영역이 갈리는 기믹이 자기 트리에서 Context 액터의 메시로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UPrimitiveComponent> TargetMesh;

	/** 남의 트리(퀘스트 스텝 등)에서 배치 대상을 지목하는 갈래. 그 대상의 상호작용을 통째로 여닫는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	FUniversalObjectLocator Target;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = false;

	/** 메시 지정 갈래에서 이 영역이 표시할 HUD 프롬프트. 코드 폴백이 없으므로 비우면 문구 없이 표시된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bEnable"))
	FText Prompt;

	/** 메시 지정 갈래에서 이 영역이 눌렸을 때 오너 기믹이 발행하는 델리게이트. 전이의 Delegate 칸에서 이것을 골라 목적지를 잇는다(끄는 노드의 것은 발행될 일이 없다). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FStateTreeDelegateDispatcher OnInteracted;
};

/**
 * 진입 시 지정 대상의 상호작용 활성/비활성을 bEnable 로 토글하고 완료한다.
 * 상태를 떠나도 되돌리지 않으므로, 다시 열 시점은 여는 상태에 이 태스크를 한 번 더 두어 에셋이 정한다.
 *
 * 영역 메시(TargetMesh) 갈래는 프롬프트와 발행 자리까지 오너 기믹에 함께 담는다 — 한 액터에 상호작용 영역이 여럿인 기믹이 쓴다.
 * 이 노드의 OnInteracted 를 지목하는 전이는 이 노드가 있는 상태나 그 하위 상태에 두어야 한다 — 바인딩이 볼 수 있는 범위가 루트에서 전이가 달린 상태까지의 경로뿐이라, 부모 상태의 전이는 자식의 발행자를 지목하지 못한다.
 *
 * 액터(Target) 갈래는 토글을 대상이 계약(IWxInteractable)으로 스스로 수행하므로 대상 타입을 알 필요가 없다.
 * 대상이 아직 스트리밍 인 되지 않았으면 그때까지만 Running 으로 남는다.
 * 값을 복제하지 않으므로 서버가 곧 클라인 싱글/리슨 호스트가 전제다.
 * 걸어 둔 토글이 대상의 재로드로 되돌아가는 것까지는 지키지 않는다.
 */
USTRUCT(meta = (DisplayName = "상호작용 켜기", Category = "Wx"))
struct FWxStateTreeTask_EnableInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableInteractionInstanceData;

	FWxStateTreeTask_EnableInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	EStateTreeRunStatus ApplyTargetInteraction(const FStateTreeExecutionContext& Context, const FInstanceDataType& Instance) const;

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FString GetTargetDisplayName(const FUniversalObjectLocator& Locator) const;
#endif
};
