// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxTreasureChest.h"

#include "Components/SkeletalMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxTreasureChest::AWxTreasureChest()
{
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxTreasureChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxTreasureChest, State);
}

void AWxTreasureChest::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxTreasureChest::HandleInteracted);
}

void AWxTreasureChest::SetGimmickState(uint8 NewStateValue)
{
	// 베이스 CommitGimmickState(권위)가 호출하는 State 쓰기 훅. 열기 애니·인터랙션 비활성은 ST 가 State 변화를 Enum Compare 로 추종해 적용한다.
	State = static_cast<EWxChestState>(NewStateValue);
}

void AWxTreasureChest::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Open 으로 확정한다. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종하므로 비권위는 노옵.
	if (HasAuthority())
	{
		CommitGimmickState(static_cast<uint8>(EWxChestState::Open));
	}
}
