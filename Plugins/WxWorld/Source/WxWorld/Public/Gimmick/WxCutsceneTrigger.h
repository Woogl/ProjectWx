// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCutsceneTrigger.generated.h"

class ULevelSequence;
class UStaticMeshComponent;

/**
 * 컷신 트리거.
 * 플레이어가 상호작용하면 지정된 Level Sequence 를 재생하고, 재생 동안 플레이어 입력이 막힌다. 재생이 끝나면 다시 상호작용할 수 있다(반복 재생).
 *
 * 상태는 자체 State 태그(Gimmick.CutsceneTrigger.*)가 권위 원천이며, 복제된다(다른 기믹과 동일한 「권위 State 태그 → StateTree 진입」 패턴).
 * 상호작용(권위)이 State 를 Playing 으로 확정한다.
 * 재생은 GimmickStateTree(ST_CutsceneTrigger)의 Play Level Sequence 가 맡고, 재생이 끝나면 그 태스크가 권위 측에서 HandleLevelSequenceFinished 로 통지해 State 를 Idle 로 되돌린다.
 * State 쓰기는 여전히 이 액터(C++)만 하므로 ST 는 진입만 한다.
 *
 *   Idle (초기) ──상호작용(권위)──> Playing ──재생 종료 통지(권위)──> Idle
 *
 * 재생·입력차단·인터랙션 토글은 GimmickStateTree 가 State 태그 이벤트로 진입한 상태에서 적용한다(재생은 Play Level Sequence, 입력은 Enable Player Input, 인터랙션은 Enable Interaction).
 * Playing 은 일시 상태라 SaveGame 보존 없음(복원 시 재생 재트리거 방지). Replicated 만 둬 멀티 동기화하며 항상 Idle 로 시작한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxCutsceneTrigger : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCutsceneTrigger();

	//~ Begin AWxGimmick — 재생 종료 시(Play Level Sequence 통지) Idle 복귀.
	virtual void HandleLevelSequenceFinished() override;
	//~ End AWxGimmick

	//~ Begin IWxSavable — 일시 상태라 복원 시 Idle 로 리셋(SaveGame 으로 끌려온 Playing 무시).
	virtual void OnWxSaveRestored() override;
	//~ End IWxSavable

	//~ Begin IWxInteractable — 상호작용 시 State 를 Playing 으로 확정(프롬프트는 ST_CutsceneTrigger 의 Enable Interaction 에서 author).
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	//~ End IWxInteractable

protected:
	virtual void BeginPlay() override;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// EditInstanceOnly + AllowPrivateAccess: 디자이너가 인스턴스마다 지정하고, StateTree 의 Play Level Sequence 가 Context 액터 프로퍼티로 바인딩하기 위한 노출.
	/** 재생할 Level Sequence 에셋. */
	UPROPERTY(EditInstanceOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULevelSequence> LevelSequence;
};
