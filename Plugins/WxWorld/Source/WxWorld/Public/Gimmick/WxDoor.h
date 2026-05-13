// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxDoor.generated.h"

class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxDoorState : uint8
{
	Closed,
	Opening,
	Open
};

/**
 * 1회성 개폐 문.
 * 콘솔과 상호작용하면 양쪽 문이 반대 방향으로 슬라이드하며 열린다. 한 번 열린 뒤에는 닫을 수 없으며 콘솔 상호작용도 비활성화된다.
 *
 * 상태 머신:
 *   Closed (초기) → 콘솔 상호작용 → Opening → Open (영구 고정)
 */
UCLASS(Abstract)
class WXWORLD_API AWxDoor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxDoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ApplyState() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> DoorRight;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	/** 문 열림 애니메이션 길이(초). */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float DoorAnimDuration = 1.f;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InstigatorActor);

	UFUNCTION()
	void OnRep_State();

	/** DoorAnimProgress 에 따라 양쪽 문 위치를 lerp. 각 문은 자신의 너비만큼 바깥쪽으로 슬라이드. */
	void UpdateDoorPositions();

	/** 문 메시의 로컬 Y 축 너비(스케일 반영). 표준 UE 도어 메시는 Y 가 너비 축. */
	float ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	EWxDoorState State = EWxDoorState::Closed;

	/** 문 애니 진행도. 0=닫힘, 1=열림. 각 머신에서 Tick 으로 로컬 누적 (복제 없음). */
	float DoorAnimProgress = 0.f;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;

	/** 각 문 메시의 Y 너비만큼 자기 바깥쪽 방향(좌: -Y, 우: +Y)으로의 슬라이드 오프셋. */
	FVector DoorLeftOpenOffset;
	FVector DoorRightOpenOffset;
};
