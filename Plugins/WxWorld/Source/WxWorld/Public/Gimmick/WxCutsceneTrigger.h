// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCutsceneTrigger.generated.h"

class ULevelSequence;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxCutsceneTriggerState : uint8
{
	/** 대기 — 초기/기본. 인터랙션 활성, 시퀀스 미재생. */
	Idle,
	/** 재생 중 — Level Sequence 재생. 인터랙션·입력은 ST 가 막는다. */
	Playing
};

/**
 * 컷신 트리거.
 * 플레이어가 상호작용하면 지정된 Level Sequence 를 재생하고, 재생 동안 플레이어 입력이 막힌다. 재생이 끝나면 다시 상호작용할 수 있다(반복 재생).
 *
 * 상태는 자체 EWxCutsceneTriggerState(State) 가 권위 원천이며, 복제된다(다른 기믹과 동일한 「권위 State enum → StateTree 추종」 패턴).
 * 상호작용(권위)이 State 를 Playing 으로 확정한다. 재생은 GimmickStateTree(ST_CutsceneTrigger)의 Wx Play Level Sequence 가 맡고, 재생이 끝나면 그 태스크가 권위 측에서 HandleLevelSequenceFinished 로 통지해 State 를 Idle 로 되돌린다. State 쓰기는 여전히 이 액터(C++)만 하므로 ST 는 추종만 한다.
 *
 *   Idle (초기) ──상호작용(권위)──> Playing ──재생 종료 통지(권위)──> Idle
 *
 * 재생·입력차단·인터랙션 토글은 GimmickStateTree 가 State 를 추종해 적용한다(재생은 Wx Play Level Sequence, 입력은 Wx Enable Player Input, 인터랙션은 Wx Enable Interaction).
 * Playing 은 일시 상태라 SaveGame 보존 없음(복원 시 재생 재트리거 방지). Replicated 만 둬 멀티 동기화하며 항상 Idle 로 시작한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxCutsceneTrigger : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCutsceneTrigger();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin AWxGimmick — 재생 종료 시(Wx Play Level Sequence 통지) Idle 복귀.
	virtual void HandleLevelSequenceFinished() override;
	//~ End AWxGimmick

protected:
	virtual void BeginPlay() override;

	//~ Begin AWxGimmick — State(EWxCutsceneTriggerState) ↔ uint8 쓰기 매핑.
	virtual void SetGimmickState(uint8 NewStateValue) override;
	//~ End AWxGimmick

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	// EditInstanceOnly + AllowPrivateAccess: 디자이너가 인스턴스마다 지정하고, StateTree 의 Wx Play Level Sequence 가 Context 액터 프로퍼티로 바인딩하기 위한 노출.
	/** 재생할 Level Sequence 에셋. */
	UPROPERTY(EditInstanceOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULevelSequence> LevelSequence;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	/**
	 * 컷신 권위 상태(복제). State 쓰기는 권위 전용(CommitGimmickState)이며, 클라는 OnRep_GimmickState 가 발행하는 이벤트로 ST 재선택을 구동한다.
	 * 일시 상태라 SaveGame 미지정(다른 기믹과 다른 점) — 복원 시 항상 Idle 로 시작한다.
	 * VisibleAnywhere + AllowPrivateAccess 는 StateTree 의 상태 선택 조건(State 비교)이 바인딩하기 위한 노출이다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx", ReplicatedUsing = OnRep_GimmickState, meta = (AllowPrivateAccess = "true"))
	EWxCutsceneTriggerState State = EWxCutsceneTriggerState::Idle;
};
