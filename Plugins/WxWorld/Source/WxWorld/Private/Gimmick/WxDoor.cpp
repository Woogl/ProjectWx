// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoor.h"

#include "Components/StateTreeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

AWxDoor::AWxDoor()
{
	DoorLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeft"));
	DoorLeft->SetupAttachment(SceneRoot);

	DoorRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorRight"));
	DoorRight->SetupAttachment(SceneRoot);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);

	DoorStateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("DoorStateTree"));
	// 모든 컴포넌트의 BeginPlay 이후 BeginPlay 에서 직접 시작한다(인터랙션 토글 등 컴포넌트 의존 순서 보장).
	DoorStateTree->SetStartLogicAutomatically(false);
}

void AWxDoor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 오프셋을 BeginPlay 보다 앞선 시점에 캐시한다. Super::BeginPlay → StartLogic 이
	// 복원 문을 Open 포즈로 스냅할 때 오프셋이 이미 준비되어 있어야 한다.
	CacheDoorPoseAnchors();
}

void AWxDoor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxDoor::HandleConsoleInteracted);

	// 모든 컴포넌트의 BeginPlay 가 끝난 뒤 StateTree 를 시작한다.
	// 시작 시 DoorTriggered 조건이 bTriggered 를 읽어 Open(스냅) 또는 Closed 로 초기 선택한다.
	DoorStateTree->StartLogic();
}

void AWxDoor::ApplyState()
{
	// bTriggered 가 true 로 전환되는 신뢰 경로(서버 MarkTriggered / 클라 OnRep_bTriggered) 에서
	// StateTree 에 발동 이벤트를 송출하여 Closed→Opening 전이를 발생시킨다.
	// 시작 시 이미 발동된 문은 StateTree 초기 선택(DoorTriggered 조건) 이 Open 으로 보내며,
	// 그 경우 여기서 보낸 이벤트는 Open 에 해당 전이가 없어 무시된다.
	if (bTriggered && DoorStateTree && DoorStateTree->IsRunning())
	{
		DoorStateTree->SendStateTreeEvent(WxGameplayTags::Event_Gimmick_Triggered);
	}
}

void AWxDoor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 1회성 발동 처리. bTriggered 가 true 로 바뀌며 MarkTriggered 가 ApplyState 를 호출해 이벤트를 송출한다.
	// 클라이언트는 OnRep_bTriggered → ApplyState 로 동일 처리되므로 비권위 분기는 노옵.
	if (HasAuthority())
	{
		MarkTriggered();
	}
}

void AWxDoor::SetConsoleInteractionEnabled(bool bEnabled)
{
	ConsoleInteraction->SetInteractionEnabled(bEnabled);
}

void AWxDoor::SetDoorOpenAlpha(float Alpha)
{
	const float Clamped = FMath::Clamp(Alpha, 0.f, 1.f);
	DoorLeft->SetRelativeLocation(DoorLeftClosedLocation + DoorLeftOpenOffset * Clamped);
	DoorRight->SetRelativeLocation(DoorRightClosedLocation + DoorRightOpenOffset * Clamped);
}

void AWxDoor::CacheDoorPoseAnchors()
{
	// 문 닫힘 위치 캐시: BP/레벨에서 배치된 상대 위치를 닫힘 기준으로 사용.
	DoorLeftClosedLocation = DoorLeft->GetRelativeLocation();
	DoorRightClosedLocation = DoorRight->GetRelativeLocation();

	// 각 문은 자기 메시 너비만큼 바깥쪽(좌: -Y, 우: +Y)으로 슬라이드.
	DoorLeftOpenOffset = FVector(0.f, -ComputeDoorWidth(DoorLeft), 0.f);
	DoorRightOpenOffset = FVector(0.f, ComputeDoorWidth(DoorRight), 0.f);
}

float AWxDoor::ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const
{
	const UStaticMesh* Mesh = DoorMesh ? DoorMesh->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return 0.f;
	}

	return Mesh->GetBounds().BoxExtent.Y * 2.f * DoorMesh->GetRelativeScale3D().Y;
}
