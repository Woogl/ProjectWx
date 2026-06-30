// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoor.h"

#include "Components/StaticMeshComponent.h"
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
	ConsoleInteraction->SetHighlightTarget(Console);

	State = WxGameplayTags::Gimmick_Door_Close;
}

void AWxDoor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxDoor::HandleConsoleInteracted);
}

void AWxDoor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 처리하며, 현재 상태의 반대 목표로 확정한다(슬라이드는 StateTree 의 Wx Component Move 가 비주얼로 처리).
	// 닫기(Open→Close)는 준비돼 있으나, Open 상태의 인터랙션 태스크가 에셋에서 비활성이라 Open 에선 이 콜백이 호출되지 않아 현재 단방향으로 동작한다.
	// 클라는 복제 State 의 OnRep 이벤트가 ST 진입을 구동하므로 비권위 분기는 노옵.
	if (!HasAuthority())
	{
		return;
	}

	const FGameplayTag NextState = (State == WxGameplayTags::Gimmick_Door_Close) ? WxGameplayTags::Gimmick_Door_Open : WxGameplayTags::Gimmick_Door_Close;
	CommitGimmickState(NextState);
}
