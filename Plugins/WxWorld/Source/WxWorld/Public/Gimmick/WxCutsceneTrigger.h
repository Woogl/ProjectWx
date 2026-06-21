// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Gimmick/WxGimmick.h"
#include "WxCutsceneTrigger.generated.h"

class ULevelSequence;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 컷신 트리거.
 * 플레이어가 상호작용하면 지정된 Level Sequence 를 재생하고, 재생 동안 플레이어 입력이 막힌다. 재생이 끝나면 다시 상호작용할 수 있다(반복 재생).
 *
 * 다른 기믹과 달리 복제 State enum 을 쓰지 않고 StateTree 이벤트 + OnComplete 전이로 구동한다(기믹 공통 "이벤트 태그 없음" 원칙의 의도적 예외).
 * 상호작용(UWxInteractionComponent::OnInteracted)은 MulticastInteracted 로 모든 피어에서 발화하므로, 각 피어가 자기 GimmickStateTree 에 PlayEventTag 이벤트를 보내 Playing 으로 전이시킨다(복제 불필요).
 * 재생이 끝나면 Wx Play Level Sequence 태스크가 Succeeded 를 반환하고, ST_CutsceneTrigger 의 OnComplete 전이가 Idle 로 되돌린다(별도 통지·타이머 없음). 재생 중엔 ST 가 인터랙션을 비활성화해 재진입을 막는다.
 *
 *   Idle (초기) ──상호작용 이벤트──> Playing ──재생 종료(OnComplete)──> Idle
 *
 * 재생·입력차단·인터랙션 토글은 GimmickStateTree(ST_CutsceneTrigger)가 적용한다(재생은 Wx Play Level Sequence, 입력은 Wx Enable Player Input, 인터랙션은 Wx Enable Interaction). 일시 상태라 SaveGame 보존 없음.
 */
UCLASS(Abstract)
class WXWORLD_API AWxCutsceneTrigger : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCutsceneTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	// EditInstanceOnly + AllowPrivateAccess: 디자이너가 인스턴스마다 지정하고, StateTree 의 Wx Play Level Sequence 가 Context 액터 프로퍼티로 바인딩하기 위한 노출.
	/** 재생할 Level Sequence 에셋. */
	UPROPERTY(EditInstanceOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULevelSequence> LevelSequence;

	/** 상호작용 시 GimmickStateTree 로 보낼 이벤트 태그. ST_CutsceneTrigger 의 Idle→Playing 전이가 이 태그를 매칭한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	FGameplayTag PlayEventTag;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);
};
