// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxLaserCorridor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxLaserCorridor::AWxLaserCorridor()
{
	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);
	// 이 메시가 곧 상호작용 영역이다. 활성/비활성은 ST 의 Wx Enable Interaction 이 이 응답을 토글해 가른다.
	Console->SetCollisionResponseToChannel(ECC_WxInteractable, ECR_Overlap);

	State = WxGameplayTags::Gimmick_LaserCorridor_Active;
}

void AWxLaserCorridor::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다. State 를 Deactivated 로 확정하면 클라는 복제 State 의 OnRep 이 ST 진입을 구동한다.
	CommitGimmickState(WxGameplayTags::Gimmick_LaserCorridor_Deactivated);
}
