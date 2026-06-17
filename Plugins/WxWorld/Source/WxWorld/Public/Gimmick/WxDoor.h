// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxDoor.generated.h"

class UStateTreeComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 1회성 개폐 문.
 * 콘솔과 상호작용하면 양쪽 문이 반대 방향으로 슬라이드하며 열린다. 한 번 열린 뒤에는 닫을 수 없으며 콘솔 상호작용도 비활성화된다.
 *
 * 상태 머신은 StateTree(DoorStateTree)가 구동한다. C++ 는 StateTree 노드가 호출할 얇은 프리미티브
 * (인터랙션 토글/문 포즈/발동 여부) 만 제공하고, 상태·전이는 ST_Door 에셋에서 author 한다.
 *
 *   Closed (초기) ──Event.Gimmick.Triggered──> Opening ──애니 완료──> Open (영구 고정)
 *   시작 시 이미 발동(bTriggered)된 문은 조건 초기 선택으로 Opening 없이 Open 으로 스냅한다.
 *
 * 트리거는 복제된 bTriggered(베이스) 가 true 로 전환되는 신뢰 경로(서버 MarkTriggered / 클라 OnRep) 에서
 * ApplyState 가 StateTree 이벤트를 송출하여 발생시킨다. 언릴라이어블 멀티캐스트가 아닌 복제 프로퍼티 기반이다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDoor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxDoor();

	//~ Begin StateTree 노드가 호출하는 프리미티브
	/** 콘솔 인터랙션 활성/비활성. */
	void SetConsoleInteractionEnabled(bool bEnabled);

	/** 문 개방 알파(0=닫힘, 1=열림)로 양쪽 문 위치를 갱신. */
	void SetDoorOpenAlpha(float Alpha);

	/** 문 열림 애니메이션 길이(초). */
	float GetDoorAnimDuration() const { return DoorAnimDuration; }

	/** 문이 1회성 개방 발동되었는지. (베이스 bTriggered) */
	bool IsDoorTriggered() const { return bTriggered; }
	//~ End StateTree 노드가 호출하는 프리미티브

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void ApplyState() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> DoorRight;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	/** 문 개폐 상태 머신을 구동하는 StateTree. 실행할 ST_Door 에셋은 BP_Door 에서 할당한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStateTreeComponent> DoorStateTree;

	/** 문 열림 애니메이션 길이(초). */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float DoorAnimDuration = 1.f;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InstigatorActor);

	/** 문 닫힘 위치와 개방 오프셋을 캐시. PostInitializeComponents 에서 1회 호출. */
	void CacheDoorPoseAnchors();

	/** 문 메시의 로컬 Y 축 너비(스케일 반영). 표준 UE 도어 메시는 Y 가 너비 축. */
	float ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;

	/** 각 문 메시의 Y 너비만큼 자기 바깥쪽 방향(좌: -Y, 우: +Y)으로의 슬라이드 오프셋. */
	FVector DoorLeftOpenOffset;
	FVector DoorRightOpenOffset;
};
