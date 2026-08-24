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
}

void AWxGhostTrail::BeginPlay()
{
	Super::BeginPlay();

	if (!Owner)
	{
		return;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	if (!OwnerCharacter)
	{
		return;
	}
	
	PoseableMesh->SetSkeletalMesh(OwnerCharacter->GetMesh()->SkeletalMesh);
	PoseableMesh->CopyPoseFromSkeletalComponent(OwnerCharacter->GetMesh());
	PoseableMesh->SetRelativeScale3D(OwnerCharacter->GetActorScale3D());

	if (MaterialOverride)
	{
		for (int32 i = 0; i < OwnerCharacter->GetMesh()->SkeletalMesh->Materials.Num(); i++)
		{
			PoseableMesh->SetMaterial(i, MaterialOverride);
		}
	}
 
	const float Height = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, -Height));
	SetActorRotation(OwnerCharacter->GetActorRotation() + FRotator(0.f, -90.f, 0.f));
 
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


