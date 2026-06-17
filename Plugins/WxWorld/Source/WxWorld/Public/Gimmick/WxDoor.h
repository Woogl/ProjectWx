// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxDoor.generated.h"

class UStateTreeComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxDoorState : uint8
{
	/** 닫힘 — 초기/기본. 콘솔 인터랙션 활성, 문 닫힘. */
	Closed,
	/** 개방 중 — 문이 0→1 로 슬라이드. 인터랙션 비활성. 애니 완료 시 서버가 Open 으로 승급. */
	Opening,
	/** 열림 — 문 완전 개방. Open 의 인터랙션을 켜면 닫기(Closing) 진입 가능. 현재는 에셋에서 비활성이라 단방향. */
	Open,
	/** 닫기 중 — 문이 1→0 로 슬라이드. 인터랙션 비활성. 애니 완료 시 서버가 Closed 로 승급. */
	Closing
};

/**
 * 개폐 문.
 * 콘솔과 상호작용하면 양쪽 문이 반대 방향으로 슬라이드하며 열린다.
 * 구조상 다시 닫을 수도 있으나(Open ──상호작용──> Closing ──> Closed), 현재는 Open 상태의 인터랙션을 에셋에서 비활성화해 단방향(열기 전용)으로 동작한다.
 * Open 의 DoorPose interaction 을 켜면 반복 개폐가 즉시 활성화된다(C++ 구조·핸들러는 이미 준비됨).
 *
 * 상태는 자체 EWxDoorState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * base bTriggered 는 사용하지 않는다.
 * StateTree(DoorStateTree)는 State 를 읽어 비주얼을 렌더하고 전이를 구동하는 상태머신이며, 상태·전이는 ST_Door 에셋에서 author 한다.
 *
 *   Closed (초기) ──상호작용──> Opening ──애니 완료(서버)──> Open ──상호작용(현재 게이트 off)──> Closing ──애니 완료(서버)──> Closed
 *
 * 전이는 "State 변경 → Event.Gimmick.StateChanged 송출 → StateTree Root 재선택" 한 메커니즘으로 통일된다(서버/클라 동일).
 * State 는 서버 권위·복제이며 클라는 SetDoorState 권위 게이트로 쓰지 않는다.
 * 시작/복원 시엔 DoorStateIs 조건이 현재 State 로 초기 선택(복원 시 Open 스냅 등)한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDoor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxDoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin StateTree 노드가 호출하는 프리미티브
	/** 콘솔 인터랙션 활성/비활성. */
	void SetConsoleInteractionEnabled(bool bEnabled);

	/** 문 개방 알파(0=닫힘, 1=열림)로 양쪽 문 위치를 갱신. */
	void SetDoorOpenAlpha(float Alpha);

	/** 문 슬라이드 애니메이션 길이(초). */
	float GetDoorAnimDuration() const { return DoorAnimDuration; }

	/** 현재 문 개방 알파(0=닫힘, 1=열림). DoorPose 가 현재→목표 보간의 시작점으로 읽는다. */
	float GetDoorOpenAlpha() const { return CurrentOpenAlpha; }

	/** 현재 문 상태. StateTree 의 DoorStateIs 조건이 상태 선택에 사용. */
	EWxDoorState GetDoorState() const { return State; }

	/** 권위 측에서 State 를 전환하고 ApplyState 로 StateTree 를 재선택. 동일값/비권위면 노옵. DoorSlide 태스크의 애니 완료 승급에도 사용. */
	void SetDoorState(EWxDoorState NewState);
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

	/** 문 슬라이드 애니메이션 길이(초). */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float DoorAnimDuration = 1.f;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InstigatorActor);

	UFUNCTION()
	void OnRep_State();

	/** 문 닫힘 위치와 개방 오프셋을 캐시. PostInitializeComponents 에서 1회 호출. */
	void CacheDoorPoseAnchors();

	/** 문 메시의 로컬 Y 축 너비(스케일 반영). 표준 UE 도어 메시는 Y 가 너비 축. */
	float ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const;

	/** 도어 권위/영속 상태. base bTriggered 대신 사용. */
	UPROPERTY(ReplicatedUsing = OnRep_State, SaveGame)
	EWxDoorState State = EWxDoorState::Closed;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;

	/** 각 문 메시의 Y 너비만큼 자기 바깥쪽 방향(좌: -Y, 우: +Y)으로의 슬라이드 오프셋. */
	FVector DoorLeftOpenOffset;
	FVector DoorRightOpenOffset;

	/** 현재 적용된 개방 알파(0~1). SetDoorOpenAlpha 가 갱신하며, DoorPose 의 보간 시작점이 된다. */
	float CurrentOpenAlpha = 0.f;
};
