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
	/** 닫힘 — 초기/기본. 콘솔 인터랙션 활성, 문 닫힘. 여닫는 "확정 목표"이며 슬라이드 도중도 이 값을 유지한다. */
	Close,
	/** 열림 — 문 개방. 콘솔 인터랙션은 에셋에서 비활성이라 현재 단방향. 여닫는 "확정 목표"이며 슬라이드 도중도 이 값을 유지한다. */
	Open
};

/**
 * 개폐 문.
 * 콘솔과 상호작용하면 양쪽 문이 반대 방향으로 슬라이드하며 열린다.
 * 구조상 다시 닫을 수도 있으나(Open ──상호작용──> Close), 현재는 Open 상태의 인터랙션을 에셋에서 비활성화해 단방향(열기 전용)으로 동작한다.
 * Open 의 DoorInteraction 을 켜면 반복 개폐가 즉시 활성화된다(C++ 구조·핸들러는 이미 준비됨).
 *
 * 상태는 자체 EWxDoorState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * State 는 "여닫는 확정 목표"라 상호작용 시점에 곧장 최종값(Open/Close)으로 확정되고, 슬라이드 애니는 그 목표를 향한 순수 비주얼로 StateTree 안에서만 처리된다(별도 전이 상태·애니 완료 승급 없음).
 * base bTriggered 는 사용하지 않는다.
 * StateTree(DoorStateTree)는 State 를 읽어 비주얼을 렌더하고 전이를 구동하는 상태머신이며, 상태·전이는 ST_Door 에셋에서 author 한다.
 *
 *   Close (초기) ──상호작용──> Open ──(양방향 시)상호작용──> Close
 *
 * 전이는 "권위 State 변경 → DoorPose 가 State 변화를 감지해 Succeeded 반환 → On Succeeded → Root 재선택" 한 메커니즘으로 구동된다(이벤트 태그 없음).
 * State 는 서버 권위·복제이며 클라는 SetDoorState 권위 게이트로 쓰지 않는다. 클라 전이는 복제된 State 변화가 게이트한다(서버가 전이의 클럭).
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

	/** 권위 측에서 State 를 전환. 동일값/비권위면 노옵. 콘솔 상호작용이 목표를 확정하는 유일한 호출처. 전이는 DoorPose 가 State 변화를 감지해 구동. */
	void SetDoorState(EWxDoorState NewState);
	//~ End StateTree 노드가 호출하는 프리미티브

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

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

	/** 문 닫힘 위치와 개방 오프셋을 캐시. PostInitializeComponents 에서 1회 호출. */
	void CacheDoorPoseAnchors();

	/** 문 메시의 로컬 Y 축 너비(스케일 반영). 표준 UE 도어 메시는 Y 가 너비 축. */
	float ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const;

	/** 도어 권위/영속 상태. base bTriggered 대신 사용. 클라는 DoorPose 가 매 틱 폴링하므로 RepNotify 불필요. */
	UPROPERTY(Replicated, SaveGame)
	EWxDoorState State = EWxDoorState::Close;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;

	/** 각 문 메시의 Y 너비만큼 자기 바깥쪽 방향(좌: -Y, 우: +Y)으로의 슬라이드 오프셋. */
	FVector DoorLeftOpenOffset;
	FVector DoorRightOpenOffset;

	/** 현재 적용된 개방 알파(0~1). SetDoorOpenAlpha 가 갱신하며, DoorPose 의 보간 시작점이 된다. */
	float CurrentOpenAlpha = 0.f;
};
