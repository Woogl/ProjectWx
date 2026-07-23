// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCheckPoint.generated.h"

class UGameplayEffect;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 체크 포인트(다크소울 모닥불).
 * 플레이어가 상호작용하면 HP를 최대치로 회복시키고, 충전형 아이템을 리필하며, 적을 리스폰하고, 게임을 저장한다.
 * HealEffect 프로퍼티에 HP를 MaxHP로 설정하는 GameplayEffect를 지정해야 한다.
 *
 * AWxGimmick 자식이다. 상호작용 시 State 를 Lit(불 켜짐)으로 확정하며, 이 상태는 복제 + SaveGame 으로 지속된다(한 번 불이 붙으면 재로드 후에도 유지).
 * 불 켜짐 비주얼은 GimmickStateTree(자식 BP 에서 ST 에셋 할당)가 Lit 상태에서 적용한다.
 *
 * 부활 지점을 따로 등록하지는 않는다: 재개 지점은 세이브 플러시가 저장 시점의 플레이어 위치로 캡처하고, 상호작용 시 플레이어는 이 앞에 서 있으므로 그 값이 곧 이 체크포인트 자리다.
 * 그래서 오토세이브가 여기뿐인 한 사망 부활도 마지막으로 불을 켠 체크포인트가 된다. 스폰은 UWxPlayerSpawnComponent 가 담당한다.
 * 신규 세션(저장 없음) 시작지점은 체크포인트가 아니라 레벨에 배치된 일반 APlayerStart(엔진 기본 ChoosePlayerStart)가 담당한다.
 */
UCLASS(Abstract)
class AWxCheckPoint : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCheckPoint();

	//~ Begin IWxInteractable — 상호작용 시 State 를 Lit 으로 확정하고 힐·충전·리스폰·세이브를 수행(프롬프트는 베이스 InteractionPrompt).
	virtual void OnInteracted(AActor* Interactor, UActorComponent* Source) override;
	//~ End IWxInteractable

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 태스크(불 켜짐 비주얼·인터랙션 토글)가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 상호작용 시 적용할 회복 GameplayEffect. HP를 MaxHP로 설정하는 GE를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UGameplayEffect> HealEffect;
};
