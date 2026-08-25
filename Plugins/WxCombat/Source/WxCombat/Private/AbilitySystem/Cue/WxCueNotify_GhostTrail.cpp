// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Cue/WxCueNotify_GhostTrail.h"

#include "WxCombatModule.h"
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

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("GhostTrail: Owner '%s'가 Character가 아니라 잔상을 만들 수 없다."), *GetNameSafe(GetOwner()));
		return;
	}

	// ACharacter::Mesh는 Optional 서브오브젝트라 널일 수 있고, CopyPoseFromSkeletalComponent는 널을 막아 주지 않는다.
	USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh();
	if (!OwnerMesh)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("GhostTrail: Character '%s'에 스켈레탈 메시가 없어 잔상을 만들 수 없다."), *GetNameSafe(OwnerCharacter));
		return;
	}

	const float Height = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	SetActorLocation(OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, -Height));
	SetActorRotation(OwnerCharacter->GetActorRotation() + FRotator(0.f, -90.f, 0.f));
	SetActorScale3D(OwnerCharacter->GetActorScale3D());

	PoseableMesh->SetSkinnedAsset(OwnerMesh->GetSkeletalMeshAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(OwnerMesh);
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


