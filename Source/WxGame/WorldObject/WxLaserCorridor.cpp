// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxLaserCorridor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

AWxLaserCorridor::AWxLaserCorridor()
{
	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);

	State = WxGameplayTags::Gimmick_LaserCorridor_Active;
}

void AWxLaserCorridor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxLaserCorridor::HandleConsoleInteracted);
}

void AWxLaserCorridor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Disabled 로 확정한다. 클라는 복제 State 의 OnRep 이벤트가 ST 진입을 구동하므로 비권위는 노옵.
	if (HasAuthority())
	{
		CommitGimmickState(WxGameplayTags::Gimmick_LaserCorridor_Deactivated);
	}
}
