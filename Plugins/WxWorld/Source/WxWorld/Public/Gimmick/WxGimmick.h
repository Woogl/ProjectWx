// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
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
 * 발동/영속 상태(State 태그, 복제 + SaveGame)는 베이스가 소유한다. 자식은 기본값(생성자)과 인터랙션 핸들러만 제공한다(CutsceneTrigger 는 일시 상태라 복원 시 Idle 로 리셋).
 *
 * 상태 구동 패턴(전 기믹 공통):
 *  - State 쓰기는 무조건 서버 권위다. 인터랙션 핸들러 등 액터 측 콜백은 CommitGimmickState 로만 State 를 확정하며, 이 단일 진입점이 권위 가드를 적용한다(클라는 State 를 쓰지 않는다).
 *  - 자식의 State 태그는 복제된다. State 가 바뀌면 권위(CommitGimmickState)·클라(OnRep_GimmickState) 양쪽이 그 상태 태그를 ST 이벤트로 발행하고, GimmickStateTree(실행할 ST 에셋은 자식 BP 에서 할당) 가 그 상태의 Required Event to Enter 로 자식 상태를 진입해 비주얼(이동/애니)·인터랙션 토글·사이드이펙트(FX/스폰 등) 를 적용한다(서버/클라 동일). 클라가 비주얼을 로컬로 선반영하더라도 복제 State 의 재선택으로 수렴한다(서버 권위 우선).
 *  - GimmickStateTree 는 자동 시작한다. 기본(resting) 상태는 Required Event 없이 시작 시 선택되고, 비기본 상태는 State 태그 이벤트로 진입한다.
 *
 * WxSave 통합:
 *  - IWxSavable 구현. 자식의 UPROPERTY(SaveGame) State 필드가 슬롯에 기록된다.
 *  - 복원 시 BeginPlay(월드 초기화 복원)·OnWxSaveRestored(스트리밍 인) 가 저장된 State 태그를 Gimmick.Restore 마커와 함께 ST 이벤트로 발행한다. 마커가 있으면 일회성 노드들이 이 진입을 라이브 발동이 아닌 복원으로 보아 스냅·스킵한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxGimmick : public AActor, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxGimmick();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 권위 측에서 State 를 NewState(상태 태그)로 확정한다. 비권위면 노옵.
	 * State 에 직접 대입하며, 인터랙션 핸들러 등 액터 측 콜백이 호출하는 단일 서버 권위 쓰기 진입점이다. 클라는 복제된 State 를 추종한다.
	 */
	void CommitGimmickState(FGameplayTag NewState);

	/** 현재 권위 State 태그. ST 조건 'Wx Gimmick State Is' 등이 읽는다. */
	FGameplayTag GetGimmickState() const { return State; }

	/**
	 * Wx Play Level Sequence 태스크가 재생 종료 시 권위 측에서 호출하는 통지 진입점. 기본 노옵이다.
	 * 시퀀스 종료를 아는 주체는 그것을 재생·폴링하는 태스크뿐이라, 태스크가 소유 기믹에 직접 통지한다.
	 * 자식은 이를 받아 CommitGimmickState 로 State 전이를 구동한다(예: 컷신 종료 후 Idle 복귀).
	 * State 쓰기는 여전히 자식(C++)만 한다.
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
	virtual void BeginPlay() override;

	/**
	 * 라이브 State 변경을 GimmickStateTree 에 통지한다 — 현재 상태 태그를 ST 이벤트로 보내 그 상태의 Required Event 전이를 구동한다.
	 * 베이스의 복제 State(ReplicatedUsing=OnRep_GimmickState)가 클라에서 갱신될 때 호출되며, 권위 측은 CommitGimmickState 가 직접 호출해 서버·클라가 같은 통지 로직을 공유한다(서버 OnRep 미발화를 메우는 RepNotify 관용구). 트리 미실행 중엔 노옵이다.
	 */
	UFUNCTION()
	void OnRep_GimmickState();

	/**
	 * 기믹의 권위 상태(상태 태그). 복제 + SaveGame. 쓰기는 권위 전용(CommitGimmickState)이며, OnRep 이 이 태그를 ST 이벤트로 발행해 클라가 ST 진입을 추종한다.
	 * 기본값은 각 자식이 생성자에서 지정한다(예: State = WxGameplayTags::Gimmick_Door_Close). State 태그가 곧 그 상태의 Required Event to Enter 다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_GimmickState, SaveGame, VisibleAnywhere,  meta = (AllowPrivateAccess = "true"))
	FGameplayTag State;

	/** 모든 자식 컴포넌트의 부착 베이스. 자식 클래스는 SetRootComponent 호출 없이 SceneRoot 에 SetupAttachment 한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 * 기믹 상태머신을 구동하는 StateTree. 실행할 ST 에셋은 자식 BP 에서 할당한다.
	 * 복제 State 태그를 Required Event to Enter 로 진입한다. 자동 시작하며, 초기 진입 스냅은 각 태스크가 자체 수행하므로 자식은 StartLogic 을 호출하지 않는다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStateTreeComponent> StateTree;

#if WITH_EDITORONLY_DATA
	/** 에디터 정면 표시용 화살표. 베이스에서 SceneRoot 에 부착 완료. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

private:
	/**
	 * 현재 상태 태그를 GimmickStateTree 로 보내 그 상태의 Required Event 전이를 구동한다(트리 실행 중일 때만).
	 * bRestoreEntry 면 Gimmick.Restore 마커를 함께 보내, 일회성 노드가 이 진입을 라이브 발동이 아닌 복원(스냅·스킵)으로 처리하게 한다.
	 */
	void SendGimmickStateEvent(bool bRestoreEntry);

	/** WxSave 슬롯 레코드의 안정적 키. 에디터에서 부여되어 에셋에 직렬화되고, 런타임/세션 간 불변이다. */
	UPROPERTY()
	FGuid WxSaveId;
};
