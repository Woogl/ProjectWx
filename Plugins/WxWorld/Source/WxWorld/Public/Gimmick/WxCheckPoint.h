// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCheckPoint.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * 체크 포인트(다크소울 모닥불).
 * 플레이어가 상호작용하면 HP를 최대치로 회복시키고, 충전형 아이템을 리필하며, 적을 리스폰하고, 게임을 저장한다.
 *
 * 넷 다 ST_CheckPoint 의 Lit 상태 태스크가 수행한다 — 'Apply Gameplay Effect To Interactor'(회복 GE 지정), 'Refill Item Charges', 'Respawn Spawners', 'Save Game'.
 * 그래서 이 클래스에는 상호작용 로직도, 회복 GE 프로퍼티도 없다. 리필 태스크는 인벤토리 도메인(WxInventory)이 제공하며, 기믹이 인벤토리를 직접 참조하지 않고 ST 에셋에서 조립한다.
 * Lit 상태의 Tag 가 곧 저장 값이라 한 번 붙인 불은 재로드 후에도 유지되고, 이미 Lit 인 상태에서 다시 쉬면 그 상태로 되돌아오는 전이가 회복·리스폰을 다시 돌린다.
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

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 태스크(불 켜짐 비주얼·인터랙션 토글)가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 부활해서 서게 될 자리. 기본은 루트와 같은 자리이며, 메시와 독립이라 토치를 두고 서는 위치·방향만 옮길 수 있다. 'Save Game' 태스크가 여기로 바인딩된다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> ResumePoint;
};
