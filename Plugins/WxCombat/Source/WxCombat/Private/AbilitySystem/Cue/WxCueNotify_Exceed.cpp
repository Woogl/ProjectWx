// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Exceed.h"
#include "Weapon/WxWeaponBase.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

AWxCueNotify_Exceed::AWxCueNotify_Exceed()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Exceed;
	bAutoDestroyOnRemove = true;
}

bool AWxCueNotify_Exceed::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	if (!MyTarget || !NiagaraSystem)
	{
		return false;
	}

	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->Deactivate();
		SpawnedNiagaraComponent = nullptr;
	}

	const AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(MyTarget);
	if (!Weapon)
	{
		return false;
	}

	USkeletalMeshComponent* WeaponMesh = Weapon->GetMesh();
	if (!WeaponMesh)
	{
		return false;
	}

	// SpawnSystemAttached는 Outer = AttachToComponent->GetOwner() 로 NewObject하므로
	// 결과적으로 Niagara 컴포넌트가 무기 액터의 자식으로 생성된다.
	SpawnedNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		WeaponMesh,
		WeaponAttachSocket,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true);

	return true;
}

bool AWxCueNotify_Exceed::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->Deactivate();
		SpawnedNiagaraComponent = nullptr;
	}

	return true;
}
