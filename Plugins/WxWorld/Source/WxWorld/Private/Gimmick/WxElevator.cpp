// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

AWxElevator::AWxElevator()
{
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(SceneRoot);
	// 두 끝점(Start/End)을 잇는 열린 경로.
	// Spline Move 는 각 상태가 가리키는 끝점 포인트 거리만 목표하므로 폐합 구간은 쓰지 않는다.
	SplineComponent->SetClosedLoop(false);

	PlatformRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlatformRoot"));
	PlatformRoot->SetupAttachment(SceneRoot);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(PlatformRoot);

	DoorLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeft"));
	DoorLeft->SetupAttachment(PlatformRoot);

	DoorRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorRight"));
	DoorRight->SetupAttachment(PlatformRoot);

	PlatformInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("PlatformInteraction"));
	PlatformInteraction->SetupAttachment(PlatformRoot);
	// 부착 부모(PlatformRoot)는 메시가 아니므로 강조 대상·볼륨은 PlatformMesh 로 명시한다.
	PlatformInteraction->SetHighlightTarget(PlatformMesh);
	PlatformInteraction->SetCollisionVolume(PlatformMesh);

	CallConsoleA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CallConsoleA"));
	CallConsoleA->SetupAttachment(SceneRoot);

	CallConsoleAInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("CallConsoleAInteraction"));
	CallConsoleAInteraction->SetupAttachment(CallConsoleA);
	CallConsoleAInteraction->SetHighlightTarget(CallConsoleA);

	CallConsoleB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CallConsoleB"));
	CallConsoleB->SetupAttachment(SceneRoot);

	CallConsoleBInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("CallConsoleBInteraction"));
	CallConsoleBInteraction->SetupAttachment(CallConsoleB);
	CallConsoleBInteraction->SetHighlightTarget(CallConsoleB);

	State = WxGameplayTags::Gimmick_Elevator_Closed;
}

void AWxElevator::OnInteracted(AActor* Interactor, UActorComponent* Source)
{
	// 서버 권위(TryInteract)에서만 호출된다. Source(상호작용 영역)로 분기한다.
	if (Source == PlatformInteraction)
	{
		// 플랫폼 위: 반대 끝점으로 토글(문이 열린 정지 상태에서만 — Closed 는 문 닫힘이라 플랫폼에 탈 수 없다).
		if (State == WxGameplayTags::Gimmick_Elevator_AtStart)
		{
			CommitGimmickState(WxGameplayTags::Gimmick_Elevator_AtEnd);
		}
		else if (State == WxGameplayTags::Gimmick_Elevator_AtEnd)
		{
			CommitGimmickState(WxGameplayTags::Gimmick_Elevator_AtStart);
		}
	}
	else if (Source == CallConsoleAInteraction)
	{
		// Start 호출(이미 AtStart 면 동일값이라 노옵).
		CommitGimmickState(WxGameplayTags::Gimmick_Elevator_AtStart);
	}
	else if (Source == CallConsoleBInteraction)
	{
		// End 호출(이미 AtEnd 면 동일값이라 노옵).
		CommitGimmickState(WxGameplayTags::Gimmick_Elevator_AtEnd);
	}
}
