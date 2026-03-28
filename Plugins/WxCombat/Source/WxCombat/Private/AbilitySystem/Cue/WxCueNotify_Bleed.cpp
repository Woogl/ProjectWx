// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Bleed.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AWxCueNotify_Bleed::AWxCueNotify_Bleed()
{
	bAutoDestroyOnRemove = true;
}

bool AWxCueNotify_Bleed::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	if (!MyTarget || !BleedNiagaraSystem)
	{
		return false;
	}

	USceneComponent* AttachTarget = MyTarget->GetRootComponent();
	if (USkeletalMeshComponent* MeshComp = MyTarget->FindComponentByClass<USkeletalMeshComponent>())
	{
		AttachTarget = MeshComp;
	}

	SpawnedNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		BleedNiagaraSystem,
		AttachTarget,
		AttachSocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true);

	return true;
}

bool AWxCueNotify_Bleed::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->Deactivate();
		SpawnedNiagaraComponent = nullptr;
	}

	return true;
}
