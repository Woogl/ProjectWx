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
 * 부모는 상태머신을 구동하는 GimmickStateTree 와, State 의 서버 권위 쓰기 진입점(CommitGimmickState)을 공통으로 제공한다.
 * 발동/영속 상태는 각 자식이 자체 State enum(복제 + SaveGame) 으로 소유한다(Door/Elevator/콘솔/상자 등). 자식은 uint8→enum 쓰기 매핑(SetGimmickState)만 제공한다.
 *
 * 상태 구동 패턴(전 기믹 공통):
 *  - State 쓰기는 무조건 서버 권위다. 인터랙션 핸들러 등 액터 측 콜백은 CommitGimmickState 로만 State 를 확정하며, 이 단일 진입점이 권위 가드를 적용한다(클라는 State 를 쓰지 않는다).
 *  - 자식의 State enum 은 복제된다. State 가 바뀌면 권위(CommitGimmickState)·클라(OnRep_GimmickState) 양쪽이 Event.GimmickStateChanged 를 발행하고, GimmickStateTree(실행할 ST 에셋은 자식 BP 에서 할당) 가 OnEvent 전이로 자식 상태를 재선택해 비주얼(이동/애니)·인터랙션 토글·사이드이펙트(FX/스폰 등) 를 적용한다(서버/클라 동일). 클라가 비주얼을 로컬로 선반영하더라도 복제 State 의 재선택으로 수렴한다(서버 권위 우선).
 *  - GimmickStateTree 는 자동 시작한다. 초기 진입과 복원의 올바른 상태 선택·스냅(위치·포즈·애니)은 각 상태의 enter 조건과 태스크가 자체 수행하므로 자식은 명시 StartLogic 을 호출하지 않는다.
 *
 * WxSave 통합:
 *  - IWxSavable 구현. 자식의 UPROPERTY(SaveGame) State 필드가 슬롯에 기록된다.
 *  - 월드 초기화 복원은 BeginPlay 이전이라 자동 시작의 enter 조건 선택이 복원값을 스냅한다. BeginPlay 이후 복원(스트리밍 인)은 OnWxSaveRestored 가 RestartLogic 으로 트리를 재시작해 복원값으로 다시 초기 선택(=스냅)한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxGimmick : public AActor, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxGimmick();

	/**
	 * 권위 측에서 State 를 NewStateValue(원시 enum 값)로 확정한다. 비권위면 노옵.
	 * SetGimmickState 로 자식의 복제 State 에 위임하며, 인터랙션 핸들러 등 액터 측 콜백이 호출하는 단일 서버 권위 쓰기 진입점이다. 클라는 복제된 State 를 추종한다.
	 */
	void CommitGimmickState(uint8 NewStateValue);

	/**
	 * Wx Play Level Sequence 태스크가 재생 종료 시 권위 측에서 호출하는 통지 진입점. 기본 노옵이다.
	 * 시퀀스 종료를 아는 주체는 그것을 재생·폴링하는 태스크뿐이라, 태스크가 소유 기믹에 직접 통지한다. 자식은 이를 받아 CommitGimmickState 로 State 전이를 구동한다(예: 컷신 종료 후 Idle 복귀). State 쓰기는 여전히 자식(C++)만 한다.
	 */
	virtual void HandleLevelSequenceFinished() {}

	//~ Begin IWxSavable
	virtual FGuid GetWxSaveId() const override;
	virtual void OnWxSaveRestored() override;
	//~ End IWxSavable

#if WITH_EDITOR
	//~ Begin AActor — WxSaveId 를 에디터에서 1회 부여(런타임/세션 간 불변 키).
	virtual void PostActorCreated() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	//~ End AActor
#endif

protected:

	/** CommitGimmickState 가 권위 측에서 호출하는 State 쓰기 훅. 기본 노옵이며, 자식이 uint8→enum 캐스트해 자기 복제 State 에 대입한다. */
	virtual void SetGimmickState(uint8 NewStateValue) {}

	/**
	 * State 변경을 GimmickStateTree 에 통지한다(Event.GimmickStateChanged 발행 → 루트 OnEvent 가 자식 재선택).
	 * 자식의 복제 State(ReplicatedUsing=OnRep_GimmickState)가 클라에서 갱신될 때 호출되며, 권위 측은 CommitGimmickState 가 직접 호출해
	 * 서버·클라가 같은 통지 로직을 공유한다(서버 OnRep 미발화를 메우는 RepNotify 관용구).
	 * 트리가 실행 중일 때만 발행한다 — 초기 시작은 enter 조건 선택이, 복원은 OnWxSaveRestored 의 RestartLogic 이 담당하므로,
	 * 미실행 중 발행해 라이브 전이로 잘못 들어가(애니/사이드이펙트 재발동) 하지 않게 한다.
	 */
	UFUNCTION()
	void OnRep_GimmickState();

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
