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

void AWxTreasureChest::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Open 으로 확정한다. 열기 애니·인터랙션 비활성은 ST 의 Open 상태(Wx Play Animation / Wx Gimmick Interaction)가 복제 State 를 추종해 적용한다.
	if (HasAuthority())
	{
		SetChestState(EWxChestState::Open);
	}
}

void AWxTreasureChest::SetChestState(EWxChestState NewState)
{
	// State 쓰기는 권위 전용. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
}
