// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_BuffATK.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"

AWxCueNotify_BuffATK::AWxCueNotify_BuffATK()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_BuffATK;
	bAutoDestroyOnRemove = true;
}

bool AWxCueNotify_BuffATK::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	if (!MyTarget || !BuffNiagaraSystem)
	{
		return false;
	}

	USceneComponent* AttachTarget = MyTarget->GetRootComponent();
	if (USkeletalMeshComponent* MeshComp = MyTarget->FindComponentByClass<USkeletalMeshComponent>())
	{
		AttachTarget = MeshComp;
	}

	SpawnedNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		BuffNiagaraSystem,
		AttachTarget,
		AttachSocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true);

	return true;
}

bool AWxCueNotify_BuffATK::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->DeactivateImmediate();
		SpawnedNiagaraComponent = nullptr;
	}

	return true;
}
