// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Metamorphose.h"
#include "WxGameplayTags.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

AWxCueNotify_Metamorphose::AWxCueNotify_Metamorphose()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Metamorphose;
	bAutoDestroyOnRemove = true;
}

bool AWxCueNotify_Metamorphose::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	if (!TargetCharacter)
	{
		return false;
	}

	if (!MetamorphoseMesh)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = TargetCharacter->GetMesh();
	if (!MeshComp)
	{
		return false;
	}

	OriginalMesh = MeshComp->GetSkeletalMeshAsset();
	MeshComp->SetSkeletalMeshAsset(MetamorphoseMesh);

	return true;
}

bool AWxCueNotify_Metamorphose::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	if (!TargetCharacter)
	{
		return true;
	}

	USkeletalMeshComponent* MeshComp = TargetCharacter->GetMesh();
	if (!MeshComp)
	{
		return true;
	}

	MeshComp->SetSkeletalMeshAsset(OriginalMesh);
	OriginalMesh = nullptr;

	return true;
}
