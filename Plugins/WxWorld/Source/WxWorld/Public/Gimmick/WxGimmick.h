// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSavable.h"
#include "WxGimmick.generated.h"

class UArrowComponent;
class USceneComponent;
class UStateTreeComponent;

/**
 * 상호작용 가능한 월드 오브젝트의 공통 부모.
 *
 * 컴포넌트(메시/InteractionComponent 등)는 자식 클래스가 직접 들고 바인딩한다.
 * 부모는 상태머신을 구동하는 GimmickStateTree 와 인터랙션 일괄 토글(SetInteractionEnabled) 을 공통으로 제공한다.
 * 발동/영속 상태는 각 자식이 자체 State enum(복제 + SaveGame) 으로 소유한다(Door/Elevator/콘솔/상자 등).
 *
 * 상태 구동 패턴(전 기믹 공통):
 *  - C++ 는 권위 State enum 만 소유한다. 인터랙션 시 권위 측이 최종 State 를 확정하고 복제한다.
 *  - GimmickStateTree(실행할 ST 에셋은 자식 BP 에서 할당) 가 복제 State 를 Enum Compare 전이로 추종하며,
 *    비주얼(이동/애니)·인터랙션 토글·사이드이펙트(FX/스폰 등) 를 전부 적용한다(서버/클라 동일, 이벤트 태그 없음).
 *  - StartLogic 은 컴포넌트 의존 순서(인터랙션 바인딩·초기 위치 스냅 등) 보장을 위해 각 자식 BeginPlay 끝에서 호출한다.
 *
 * WxSave 통합:
 *  - IWxSavable 구현. 자식의 UPROPERTY(SaveGame) State 필드가 슬롯에 기록된다.
 *  - BeginPlay 이후 복원(스트리밍 인-스트림) 도 StateTree 전이가 복원된 State 를 폴링해 자동 추종하므로 별도 후크가 없다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxGimmick : public AActor, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxGimmick();

	//~ Begin IWxSavable
	virtual FGuid GetWxSaveId() const override;
	//~ End IWxSavable

	/** 이 기믹의 모든 인터랙션 영역(UWxInteractionComponent)을 일괄 활성/비활성. 공통 StateTree 노드(Wx Gimmick Interaction)가 호출. */
	void SetInteractionEnabled(bool bEnabled);

#if WITH_EDITOR
	//~ Begin AActor — WxSaveId 를 에디터에서 1회 부여(런타임/세션 간 불변 키).
	virtual void PostActorCreated() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	//~ End AActor
#endif

protected:

	/** 모든 자식 컴포넌트의 부착 베이스. 자식 클래스는 SetRootComponent 호출 없이 SceneRoot 에 SetupAttachment 한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 * 기믹 상태머신을 구동하는 StateTree. 실행할 ST 에셋은 자식 BP 에서 할당한다.
	 * 복제 State 를 Enum Compare 전이로 추종한다. 컴포넌트 의존 순서를 위해 자동 시작을 끄고, 자식 BeginPlay 끝에서 StartLogic 을 호출한다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStateTreeComponent> StateTree;

#if WITH_EDITORONLY_DATA
	/** 에디터 정면 표시용 화살표. 베이스에서 SceneRoot 에 부착 완료. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

private:
	/** WxSave 슬롯 레코드의 안정적 키. 에디터에서 부여되어 에셋에 직렬화되고, 런타임/세션 간 불변이다. */
	UPROPERTY()
	FGuid WxSaveId;
};
