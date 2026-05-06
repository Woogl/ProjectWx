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

	// BP에서 추가한 외형 SkeletalMeshComponent("VisualOverride" 태그)가 있으면 그 컴포넌트의 메시를 교체한다.
	// (AWxWeaponBase::AttachToCharacter 와 동일 규칙)
	for (USceneComponent* Child : MeshComp->GetAttachChildren())
	{
		if (USkeletalMeshComponent* SkelChild = Cast<USkeletalMeshComponent>(Child))
		{
			if (SkelChild->ComponentHasTag(TEXT("VisualOverride")))
			{
				MeshComp = SkelChild;
				break;
			}
		}
	}

	AffectedMeshComp = MeshComp;
	OriginalMesh = MeshComp->GetSkeletalMeshAsset();
	MeshComp->SetSkeletalMeshAsset(MetamorphoseMesh);

	return true;
}

bool AWxCueNotify_Metamorphose::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	if (USkeletalMeshComponent* MeshComp = AffectedMeshComp.Get())
	{
		MeshComp->SetSkeletalMeshAsset(OriginalMesh);
	}

	AffectedMeshComp.Reset();
	OriginalMesh = nullptr;

	return true;
}
