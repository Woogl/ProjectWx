// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxTreasureChest.h"

#include "Components/SkeletalMeshComponent.h"
#include "WxGameplayTags.h"

AWxTreasureChest::AWxTreasureChest()
{
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	// 이 메시가 곧 상호작용 영역이며, 기본 활성으로 시작한다. 이후 활성/비활성은 ST 의 Enable Interaction 이 이 집합에 넣고 빼 가른다.
	ActiveInteractionMeshes.Add(MeshComponent);

	State = WxGameplayTags::Gimmick_TreasureChest_Closed;
}

void AWxTreasureChest::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다. State 를 Open 으로 확정하면 클라는 복제 State 의 OnRep 이 ST 진입을 구동한다.
	CommitGimmickState(WxGameplayTags::Gimmick_TreasureChest_Open);
}
