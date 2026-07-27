// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCheckPoint.generated.h"

class UGameplayEffect;
class USceneComponent;
class UStaticMeshComponent;

/**
 * 체크 포인트(다크소울 모닥불).
 * 플레이어가 상호작용하면 HP를 최대치로 회복시키고, 충전형 아이템을 리필하며, 적을 리스폰한다.
 * HealEffect 프로퍼티에 HP를 MaxHP로 설정하는 GameplayEffect를 지정해야 한다.
 *
 * AWxGimmick 자식이다. 상호작용 시 State 를 Lit(불 켜짐)으로 확정하며, 이 상태는 복제 + SaveGame 으로 지속된다(한 번 불이 붙으면 재로드 후에도 유지).
 * 불 켜짐 비주얼과 게임 저장('Save Game' 태스크), 충전형 아이템 리필('Refill Item Charges' 태스크)은 GimmickStateTree(자식 BP 에서 ST 에셋 할당)가 Lit 상태에서 수행한다.
 * 리필 태스크는 인벤토리 도메인(WxInventory)이 제공한다 — 보물상자의 보상 지급('Grant Reward')과 같은 방식으로, 기믹이 인벤토리를 직접 참조하지 않고 ST 에셋에서 조립한다.
 *
 * 재개 지점은 ST 에셋이 정한다: 'Save Game' 태스크의 ResumePoint 를 이 액터의 ResumePoint 컴포넌트로 바인딩해 두었으므로, 플레이어가 어디에 서서 상호작용했든 부활 위치는 그 컴포넌트 자리로 고정된다(서는 자리·방향은 그 컴포넌트를 옮겨 잡는다).
 * 그래서 오토세이브가 여기뿐인 한 사망 부활도 마지막으로 불을 켠 체크포인트가 된다. 스폰은 UWxPlayerSpawnComponent 가 위치와 Yaw 만 써서 담당한다.
 * 신규 세션(저장 없음) 시작지점은 체크포인트가 아니라 레벨에 배치된 일반 APlayerStart(엔진 기본 ChoosePlayerStart)가 담당한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxCheckPoint : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCheckPoint();

	//~ Begin IWxInteractable — 상호작용 시 State 를 Lit 으로 확정하고 힐·리스폰을 수행(프롬프트는 ST_CheckPoint 의 Enable Interaction 에서 author).
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	//~ End IWxInteractable

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 태스크(불 켜짐 비주얼·인터랙션 토글)가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 부활해서 서게 될 자리. 기본은 루트와 같은 자리이며, 메시와 독립이라 토치를 두고 서는 위치·방향만 옮길 수 있다. 'Save Game' 태스크가 여기로 바인딩된다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> ResumePoint;

	/** 상호작용 시 적용할 회복 GameplayEffect. HP를 MaxHP로 설정하는 GE를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UGameplayEffect> HealEffect;
};
