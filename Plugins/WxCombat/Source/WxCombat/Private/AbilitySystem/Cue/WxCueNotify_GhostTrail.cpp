// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Cue/WxCueNotify_GhostTrail.h"

#include "WxCombatModule.h"
#include "WxGameplayTags.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/Character.h"

AWxGhostTrail::AWxGhostTrail()
{
	PrimaryActorTick.bCanEverTick = false;
	
	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>("PoseableMesh");
	PoseableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(PoseableMesh);
}

void AWxGhostTrail::BeginPlay()
{
	Super::BeginPlay();

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("GhostTrail: Owner '%s'가 Character가 아니라 잔상을 만들 수 없다."), *GetNameSafe(GetOwner()));
		return;
	}

	USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh();
	if (!OwnerMesh)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("GhostTrail: Owner '%s'에 메시가 없어 잔상을 만들 수 없다."), *GetNameSafe(GetOwner()));
		return;
	}

	// 포즈를 메시 기준 컴포넌트 스페이스로 복사하므로 배치 기준도 메시 트랜스폼이어야 한다 — 메시 상대 트랜스폼이 기본값이 아니면 어긋난다.
	SetActorTransform(OwnerMesh->GetComponentTransform());

	PoseableMesh->SetSkinnedAsset(OwnerMesh->GetSkeletalMeshAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(OwnerMesh);
}

UWxCueNotify_GhostTrail::UWxCueNotify_GhostTrail()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_GhostTrail;
}

void UWxCueNotify_GhostTrail::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed || !MyTarget || !GhostTrailClass)
	{
		return;
	}

	UWorld* World = MyTarget->GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = MyTarget;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWxGhostTrail* GhostTrail = World->SpawnActor<AWxGhostTrail>(GhostTrailClass, MyTarget->GetActorTransform(), Params);
	if (!GhostTrail)
	{
		return;
	}

	GhostTrail->SetLifeSpan(LifeSpan);
}


