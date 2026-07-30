// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxGimmick.generated.h"

class UArrowComponent;
class USceneComponent;
class UWxGimmickStateTreeComponent;

/**
 * 상호작용 가능한 월드 오브젝트의 공통 부모.
 *
 * 기믹의 실체는 전부 GimmickStateTree 컴포넌트에 있다 — 상태 태그(복제·SaveGame), 상호작용 계약, StateTree 구동이 그쪽에 산다.
 * 이 액터는 부착 루트와 에디터 표시만 제공하는 껍데기이며, 자식은 자기 메시를 만들어 SceneRoot 에 붙이기만 한다.
 * 그래서 신규 기믹은 이 클래스를 상속할 필요가 없다 — 아무 액터(순수 BP 포함)에 GimmickStateTree 컴포넌트를 붙이면 기믹이 된다.
 * 이미 배치된 기믹 넷(Door·Elevator·TreasureChest·CheckPoint)이 순수 BP 로 재저작되면 이 클래스도 함께 사라진다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxGimmick : public AActor
{
	GENERATED_BODY()

public:
	AWxGimmick();

protected:
	/** 모든 자식 컴포넌트의 부착 베이스. 자식 클래스는 SetRootComponent 호출 없이 SceneRoot 에 SetupAttachment 한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 기믹의 상태·상호작용·영속을 담당한다. 실행할 ST 에셋은 자식 BP 에서 할당한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxGimmickStateTreeComponent> StateTree;

#if WITH_EDITORONLY_DATA
	/** 에디터 정면 표시용 화살표. 베이스에서 SceneRoot 에 부착 완료. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
};
