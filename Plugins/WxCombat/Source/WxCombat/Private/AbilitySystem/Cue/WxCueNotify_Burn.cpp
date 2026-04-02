// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Burn.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"

AWxCueNotify_Burn::AWxCueNotify_Burn()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Burn;
	bAutoDestroyOnRemove = true;
}

bool AWxCueNotify_Burn::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	if (!MyTarget || !NiagaraSystem)
	{
		return false;
	}
	
	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->Deactivate();
	}

	USceneComponent* AttachTarget = MyTarget->GetRootComponent();
	if (USkeletalMeshComponent* MeshComp = MyTarget->FindComponentByClass<USkeletalMeshComponent>())
	{
		AttachTarget = MeshComp;
	}

	SpawnedNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachTarget,
		AttachSocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true);

	return true;
}

bool AWxCueNotify_Burn::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->Deactivate();
		SpawnedNiagaraComponent = nullptr;
	}

	return true;
}
