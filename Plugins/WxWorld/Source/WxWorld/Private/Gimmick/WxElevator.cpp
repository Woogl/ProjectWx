// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
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
	// 세 메시가 각각 독립된 상호작용 영역이며, 셋 다 기본 활성으로 시작한다. 이후 활성/비활성은 ST 의 Enable Interaction 이 영역마다 이 집합에 넣고 빼 가른다.
	ActiveInteractionMeshes.Add(PlatformMesh);

	DoorLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeft"));
	DoorLeft->SetupAttachment(PlatformRoot);

	DoorRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorRight"));
	DoorRight->SetupAttachment(PlatformRoot);

	CallConsoleA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CallConsoleA"));
	CallConsoleA->SetupAttachment(SceneRoot);
	ActiveInteractionMeshes.Add(CallConsoleA);

	CallConsoleB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CallConsoleB"));
	CallConsoleB->SetupAttachment(SceneRoot);
	ActiveInteractionMeshes.Add(CallConsoleB);

	State = WxGameplayTags::Gimmick_Elevator_Closed;
}

void AWxElevator::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다. Source(상호작용이 일어난 메시)로 영역을 가른다.
	if (Source == PlatformMesh)
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
	else if (Source == CallConsoleA)
	{
		// Start 호출(이미 AtStart 면 동일값이라 노옵).
		CommitGimmickState(WxGameplayTags::Gimmick_Elevator_AtStart);
	}
	else if (Source == CallConsoleB)
	{
		// End 호출(이미 AtEnd 면 동일값이라 노옵).
		CommitGimmickState(WxGameplayTags::Gimmick_Elevator_AtEnd);
	}
}
