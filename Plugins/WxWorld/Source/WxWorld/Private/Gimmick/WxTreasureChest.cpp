// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxTreasureChest.h"

#include "Components/SkeletalMeshComponent.h"

AWxTreasureChest::AWxTreasureChest()
{
	// 이 메시가 곧 상호작용 영역이다. 언제 켜지고 어디로 가는지는 ST 의 각 상태가 선언한다.
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
}
