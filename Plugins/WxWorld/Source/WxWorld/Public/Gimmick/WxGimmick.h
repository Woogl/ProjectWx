// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "WxInteractable.h"
#include "WxSavable.h"
#include "WxGimmick.generated.h"

class ACharacter;
class UArrowComponent;
class UPrimitiveComponent;
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
 *  - 자식의 State 태그는 복제된다. State 가 바뀌면 권위(CommitGimmickState)·클라(OnRep_GimmickState) 양쪽이 그 상태 태그를 ST 이벤트로 발행하고, GimmickStateTree(실행할 ST 에셋은 자식 BP 에서 할당) 가 그 이벤트로 자식 상태를 진입해 비주얼(이동/애니)·인터랙션 토글·사이드이펙트(FX/스폰 등) 를 적용한다(서버/클라 동일). 클라가 비주얼을 로컬로 선반영하더라도 복제 State 의 재선택으로 수렴한다(서버 권위 우선).
 *  - ST 에셋 배선은 재선택 패턴이다. Root 에 전이 하나(On Event: Gimmick → GotoState: Root)만 두면 부모 태그 계층 매칭으로 Gimmick.* 상태 태그 전부가 그 전이를 발화시켜 Root 재선택을 열고, 어느 자식으로 갈지는 각 상태의 Required Event to Enter 가 정한다. 상태를 추가해도 Root 배선은 그대로다.
 *  - GimmickStateTree 는 자동 시작한다. 기본(resting) 상태는 Required Event 없이 두어 시작 시 선택되게 하고, 비기본 상태는 State 태그를 Required Event to Enter 로 받는다. 조건 없는 상태는 재선택 때마다 항상 매칭되므로 resting 은 반드시 Root 자식 중 마지막에 둔다(위에 있으면 상태를 바꿔도 계속 resting 이 선택된다).
 *
 * WxSave 통합:
 *  - IWxSavable 구현. 자식의 UPROPERTY(SaveGame) State 필드가 슬롯에 기록된다.
 *  - 복원 시 BeginPlay(월드 초기화 복원)·OnWxSaveRestored(스트리밍 인) 가 저장된 State 태그를 StateTree.Restore 마커와 함께 ST 이벤트로 발행한다. 마커가 있으면 일회성 노드들이 이 진입을 라이브 발동이 아닌 복원으로 보아 스냅·스킵한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxGimmick : public AActor, public IWxSavable, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxGimmick();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 권위 측에서 State 를 NewState(상태 태그)로 확정한다. 비권위면 노옵.
	 * State 에 직접 대입하며, 인터랙션 핸들러 등 액터 측 콜백이 호출하는 단일 서버 권위 쓰기 진입점이다. 클라는 복제된 State 를 추종한다.
	 *
	 * 이미 NewState 인 동일값 커밋은 노옵이다 — 복제 프로퍼티가 변하지 않아 클라에선 OnRep 이 발화하지 않으므로, 권위 측만 통지를 돌리면 서버/클라 ST 가 갈린다.
	 */
	void CommitGimmickState(FGameplayTag NewState);

	/** 현재 권위 State 태그(외부 조회용). 자식은 protected State 를 직접 읽는다. */
	FGameplayTag GetGimmickState() const { return State; }

	/**
	 * 이번 상호작용의 당사자(플레이어 캐릭터)를 권위 측에서 기록한다. 비권위면 노옵.
	 * instigator 는 권위 측 OnInteracted 에서만 오므로, 자식이 그 override 에서 CommitGimmickState 직전에 호출한다(복제되어 클라가 추종).
	 * 캐릭터가 아니면(비캐릭터 상호작용) null 을 저장한다. 상호작용 이동/몽타주 태스크('Move Interactor To Target'·'Play Interactor Montage')가 이 값을 읽어 이동/재생 대상으로 삼는다.
	 */
	void SetInteractingCharacter(AActor* InActor);

	/** 이번 상호작용의 당사자(플레이어 캐릭터). 상호작용 이동/몽타주 태스크가 오너를 캐스트해 읽는다. 상호작용 중이 아니면 null. */
	ACharacter* GetInteractingCharacter() const { return InteractingCharacter; }

	/**
	 * Play Level Sequence 태스크가 재생 종료 시 권위 측에서 호출하는 통지 진입점. 기본 노옵이다.
	 * 시퀀스 종료를 아는 주체는 그것을 재생·폴링하는 태스크뿐이라, 태스크가 소유 기믹에 직접 통지한다.
	 * 자식은 이를 받아 CommitGimmickState 로 State 전이를 구동한다(예: 컷신 종료 후 Idle 복귀).
	 * State 쓰기는 여전히 자식(C++)만 한다.
	 */
	virtual void HandleLevelSequenceFinished() {}

	//~ Begin IWxInteractable
	/** 지금 켜져 있는 영역인가. 자식은 생성자에서 기본 활성 영역을 선언하고, 이후엔 'Enable Interaction' 태스크가 상태별로 넣고 뺀다. */
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;

	// 상호작용 응답은 각 구체 기믹이 override 한다. UCLASS(Abstract) 라도 CDO 는 생성되므로 순수 가상은 피하고 PURE_VIRTUAL 로 미구현 계약을 표시한다(미override 시 런타임 에러).
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override PURE_VIRTUAL(AWxGimmick::OnInteracted, );

	/** HUD 프롬프트. ST 가 그 메시에 세팅한 현재 값(CurrentInteractionPrompts)을 우선 반환하고, 없으면 GetDefaultInteractionPrompt 로 폴백한다. 우선순위는 베이스가 소유하므로 자식은 이 함수가 아니라 폴백 쪽을 override 한다. */
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

	/**
	 * Mesh 영역의 상호작용을 켜고 끈다. 'Enable Interaction' 태스크가 상태 진입 시 자기 대상 메시로 호출한다.
	 * 꺼진 영역은 IsInteractionMeshActive 가 false 를 답해 다음 스캔에서 후보에서 빠지고, 어빌리티의 서버 활성 검증에도 걸린다.
	 * 프롬프트와 마찬가지로 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetInteractionEnabled(UPrimitiveComponent* Mesh, bool bEnabled);

	/**
	 * 현재 상태에서 Mesh 영역이 표시할 상호작용 프롬프트를 로컬로 세팅한다(빈 텍스트면 그 영역의 세팅을 지운다). 'Enable Interaction' 태스크가 자기 대상 메시로 호출한다.
	 * 영역마다 따로 담으므로 한 상태가 여러 영역을 켜도 서로 덮어쓰지 않는다.
	 * 복제하지 않는다 — ST 는 각 피어에서 실행되어 EnterState 가 로컬로 이 값을 채우고, 스캐너도 로컬에서 GetInteractionPrompt 를 pull 하므로 같은 피어에서 저장·조회가 닫힌다(State 복제가 각 피어를 같은 상태로 수렴시켜 표시가 일치한다).
	 */
	void SetCurrentInteractionPrompt(UPrimitiveComponent* Mesh, const FText& InPrompt);

	//~ Begin IWxSavable
	virtual FGuid GetSaveId() const override;
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
	 * ST 가 상태별 문구를 세팅하지 않았을 때 Source 영역이 표시할 프롬프트. 기본은 기믹 하나짜리 InteractionPrompt 다.
	 * 상호작용 영역이 여럿인 기믹(예: 엘리베이터)이 이를 override 해 영역별 고정 문구를 고른다 — ST 값 우선 규칙은 베이스가 소유하므로 자식은 폴백만 신경 쓰면 된다.
	 */
	virtual FText GetDefaultInteractionPrompt(const UActorComponent* Source) const { return InteractionPrompt; }

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

	/** HUD 리스트에 표시할 기본 상호작용 프롬프트. 영역별·상태별 프롬프트가 없는 기믹/상태의 최종 폴백이다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	FText InteractionPrompt = FText::FromString(TEXT("Interact"));

	/**
	 * 지금 상호작용이 켜져 있는 영역 메시들. 멤버십 자체가 활성 상태라 따로 담는 bool 이 없다.
	 * 자식 생성자가 기본 활성 영역을 담고, 이후엔 'Enable Interaction' 태스크가 SetInteractionEnabled 로 넣고 뺀다.
	 * 로컬 전용(복제·SaveGame 아님) — 복제 State 로 구동되는 ST 가 각 피어에서 같은 값으로 수렴시킨다.
	 */
	UPROPERTY()
	TSet<TObjectPtr<UPrimitiveComponent>> ActiveInteractionMeshes;

	/**
	 * (런타임) 영역(메시)별 현재 상태의 상호작용 프롬프트. 'Enable Interaction' 태스크가 상태 진입 시 SetCurrentInteractionPrompt 로 채우고, GetInteractionPrompt 가 읽는 로컬 표시 값이다.
	 * 로컬 전용(복제·SaveGame 아님) — 복원/late-join 시 ST 재진입이 다시 세팅한다.
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, FText> CurrentInteractionPrompts;

	/**
	 * 이번 상호작용의 당사자(플레이어 캐릭터). 복제되지만 비영속(SaveGame 아님) — 상호작용 순간에만 유효하다.
	 * 권위 측이 SetInteractingCharacter 로만 쓰고, 상호작용 이동/몽타주 태스크가 GetInteractingCharacter 로 읽는다.
	 * ST 바인딩 대상이 아니므로 AllowPrivateAccess 를 붙이지 않는다 — 붙이면 타입과 무관하게 모든 바인딩 피커 목록에 오른다. VisibleAnywhere 는 디테일 패널 디버깅용으로 유지.
	 */
	UPROPERTY(Replicated, VisibleAnywhere)
	TObjectPtr<ACharacter> InteractingCharacter;

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
	 * bRestoreEntry 면 StateTree.Restore 마커를 함께 보내, 일회성 노드가 이 진입을 라이브 발동이 아닌 복원(스냅·스킵)으로 처리하게 한다.
	 */
	void SendGimmickStateEvent(bool bRestoreEntry);

	/** WxSave 슬롯 레코드의 안정적 키. 에디터에서 부여되어 에셋에 직렬화되고, 런타임/세션 간 불변이다. */
	UPROPERTY()
	FGuid SaveId;
};
