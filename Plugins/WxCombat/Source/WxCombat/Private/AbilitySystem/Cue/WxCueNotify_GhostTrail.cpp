// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Cue/WxCueNotify_GhostTrail.h"

#include "WxGameplayTags.h"
#include "Components/CapsuleComponent.h"
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

	if (!Owner)
	{
		return;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	const float Height = OwnerCharacter ? OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	
	SetActorLocation(OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, -Height));
	SetActorRotation(OwnerCharacter->GetActorRotation() + FRotator(0.f, -90.f, 0.f));
	SetActorScale3D(OwnerCharacter->GetActorScale3D());
	
	PoseableMesh->SetSkinnedAsset(OwnerCharacter->GetMesh()->GetSkeletalMeshAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(OwnerCharacter->GetMesh());
}

void AWxGhostTrail::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

UWxCueNotify_GhostTrail::UWxCueNotify_GhostTrail()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_GhostTrail;
}

void UWxCueNotify_GhostTrail::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (!GhostTrailClass)
	{
		return;
	}
		
	FActorSpawnParameters Params;
	Params.Owner = MyTarget;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	SpawnedGhostTrail = MyTarget->GetWorld()->SpawnActor<AWxGhostTrail>(GhostTrailClass, MyTarget->GetActorTransform(), Params);
	SpawnedGhostTrail->SetLifeSpan(LifeSpan);
	
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);
}


