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

	/** 토글할 상호작용 영역(대상 메시). 상태에 따라 영역이 갈리는 기믹이 자기 트리에서 Context 액터의 메시(예: Console)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UPrimitiveComponent> TargetMesh;

	/** 토글할 대상 액터 지정. 남의 트리(퀘스트 스텝 등)에서 배치 대상을 지목할 때 쓰며, 그 대상의 상호작용을 통째로 여닫는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	FUniversalObjectLocator Target;

	/** 진입 시 위 대상의 상호작용 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = false;

	/** 상호작용을 켤 때 이 영역이 표시할 HUD 프롬프트. 오너 기믹의 GetInteractionPrompt 로 pull 된다. 코드 폴백이 없으므로 비우면 문구 없이 표시된다. 메시 지정 갈래에서 bEnable 일 때만 의미가 있다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bEnable"))
	FText Prompt;

	/** 이 영역이 눌렸을 때 오너 기믹이 발행하는 델리게이트. 전이의 Delegate 칸에서 이것을 골라 "이 영역이 눌리면 저기로" 를 잇는다(끄는 노드의 것은 발행될 일이 없다). 메시 지정 갈래 전용이다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FStateTreeDelegateDispatcher OnInteracted;
};

/**
 * 진입 시 지정 대상의 상호작용 활성/비활성을 bEnable 로 토글한다 — 꺼진 대상은 스캔 후보에서 빠져 프롬프트조차 뜨지 않고, 서버 활성 검증에도 걸려 상호작용이 성립하지 않는다.
 * 각 상태가 자기 상호작용 가용 여부를 명시하도록 상태마다 둔다(직접 복원 시에도 일관). 상태를 떠나도 되돌리지 않으므로 다시 열 시점은 여는 상태에 이 태스크를 bEnable=true 로 한 번 더 두어 에셋이 정한다.
 * 포즈/이동 등과 직교하는 단일 책임 태스크. 토글할 것이 여럿이면 노드를 여럿 얹는다. 둘 다 비워 두면 무동작이다(옵션 파라미터로 쓰는 재사용 스텝의 계약).
 *
 * 대상은 두 가지 방식으로 지목하며, 걸고 나면 완료한다.
 *  - 영역 메시(TargetMesh): 한 액터에 상호작용 영역이 여럿인 기믹이 쓴다. 오너 기믹에 프롬프트와 발행 자리를 함께 담아, "이 상태가 상호작용 가능한가 + 문구는 무엇인가 + 눌리면 무엇이 발행되는가"를 한 자리에서 author 한다.
 *    눌리면 이 노드의 OnInteracted 가 발행되고, 그것을 받아 어느 상태로 갈지는 전적으로 에셋의 전이가 정한다 — 이 노드는 목적지를 모른다.
 *    영역이 여럿이라 갈 곳이 갈리는 상태는 전이가 각각 다른 노드의 OnInteracted 를 지목하면 갈린다. 그 전이는 이 노드가 있는 상태나 그 하위 상태에 두어야 한다 — 바인딩이 볼 수 있는 범위가 루트에서 전이가 달린 상태까지의 경로뿐이라, 부모 상태의 전이는 자식의 발행자를 지목하지 못한다.
 *    프롬프트·발행자는 영역별로 담기므로 한 상태가 여러 영역을 켜도 서로 덮어쓰지 않는다. 끄는 상태에서는 그 영역의 세팅을 지워 다시 켤 때 이전 상태의 문구를 물려받지 않는다.
 *  - 액터(Target): 남의 트리에서 배치 대상을 지목하는 갈래. 토글은 대상이 계약(IWxInteractable)으로 스스로 수행하므로 대상 타입을 알 필요가 없다.
 *    대상이 아직 스트리밍 인 되지 않았으면 그때까지만 Running 으로 남는다 — 진입 시점에 언로드라고 해서 놓치지 않는다.
 *    값을 복제하지 않으므로 서버가 곧 클라인 싱글/리슨 호스트가 전제다(다른 크로스모듈 노드와 같은 전제).
 *    걸어 둔 토글이 대상의 재로드로 되돌아가는 것까지는 지키지 않는다 — 다시 필요하면 그것을 켜는 상태를 다시 밟는다.
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
	/** 액터 지정 갈래의 적용. 대상이 아직 로드되지 않았을 때만 Running 으로 남아 다음 틱에 다시 시도한다. */
	EStateTreeRunStatus ApplyTargetInteraction(const FStateTreeExecutionContext& Context, const FInstanceDataType& Instance) const;

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 미지정. */
	FString GetTargetDisplayName(const FUniversalObjectLocator& Locator) const;
#endif
};
