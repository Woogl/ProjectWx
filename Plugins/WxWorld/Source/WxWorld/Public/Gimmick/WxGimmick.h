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
 * 부모는 상태머신을 구동하는 GimmickStateTree 를 공통으로 제공한다.
 * 발동/영속 상태는 각 자식이 자체 State enum(복제 + SaveGame) 으로 소유한다(Door/Elevator/콘솔/상자 등).
 *
 * 상태 구동 패턴(전 기믹 공통):
 *  - C++ 는 권위 State enum 만 소유한다. 인터랙션 시 권위 측이 최종 State 를 확정하고 복제한다.
 *  - GimmickStateTree(실행할 ST 에셋은 자식 BP 에서 할당) 가 복제 State 를 Enum Compare 전이로 추종하며,
 *    비주얼(이동/애니)·인터랙션 토글·사이드이펙트(FX/스폰 등) 를 전부 적용한다(서버/클라 동일, 이벤트 태그 없음).
 *  - GimmickStateTree 는 자동 시작한다. 초기 진입 스냅(위치·포즈·애니)은 각 태스크가 자체 수행하므로 자식은 명시 StartLogic 을 호출하지 않는다.
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

	/** Wx Set State 태스크가 권위 측에서 ST 로부터 기믹 State 를 확정하도록 호출하는 베이스 훅. 기본은 노옵이며, 채택 기믹이 오버라이드해 원시 값을 자기 State enum 으로 캐스트해 SetXxxState 로 위임한다. */
	virtual void SetGimmickState(uint8 NewStateValue) {}

	//~ Begin IWxSavable
	virtual FGuid GetWxSaveId() const override;
	//~ End IWxSavable

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
	 * 복제 State 를 Enum Compare 전이로 추종한다. 자동 시작하며, 초기 진입 스냅은 각 태스크가 자체 수행하므로 자식은 StartLogic 을 호출하지 않는다.
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
