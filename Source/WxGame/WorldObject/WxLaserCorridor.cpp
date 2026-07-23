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
	ConsoleInteraction->SetHighlightTarget(Console);

	State = WxGameplayTags::Gimmick_LaserCorridor_Active;
}

void AWxLaserCorridor::OnInteracted(AActor* Interactor, UActorComponent* Source)
{
	// 서버 권위(TryInteract)에서만 호출된다. State 를 Deactivated 로 확정하면 클라는 복제 State 의 OnRep 이 ST 진입을 구동한다.
	CommitGimmickState(WxGameplayTags::Gimmick_LaserCorridor_Deactivated);
}
