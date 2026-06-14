// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxTreasureChest.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"

AWxTreasureChest::AWxTreasureChest()
{
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxTreasureChest::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxTreasureChest::HandleInteracted);

	ApplyState();
}

void AWxTreasureChest::ApplyState()
{
	if (!bTriggered)
	{
		return;
	}

	InteractionComponent->SetInteractionEnabled(false);

	// 이미 발동된 채 로드/복원/늦참가된 경우: 열린 최종 포즈로 스냅한다.
	// 라이브 발동(HandleInteracted)이 동시에 재생 중이면 그쪽에 맡기고 스냅을 건너뛴다.
	if (OpenAnimation && !MeshComponent->IsPlaying())
	{
		MeshComponent->SetAnimation(OpenAnimation);
		MeshComponent->SetPosition(OpenAnimation->GetPlayLength(), false);
	}
}

void AWxTreasureChest::HandleInteracted(AActor* InstigatorActor)
{
	// OnInteracted 는 Multicast 라 라이브 발동 순간 서버+모든 클라에서 실행된다.
	// 전 머신에서 열기 애니메이션을 재생하고, 복원/리로드 경로는 ApplyState 가 끝 포즈로 스냅한다.
	if (OpenAnimation)
	{
		MeshComponent->PlayAnimation(OpenAnimation, false);
	}

	if (HasAuthority())
	{
		MarkTriggered();
	}
}
