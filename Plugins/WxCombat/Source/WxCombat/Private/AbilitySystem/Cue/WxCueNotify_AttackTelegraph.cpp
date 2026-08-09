// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_AttackTelegraph.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"

AWxCueNotify_AttackTelegraph::AWxCueNotify_AttackTelegraph()
{
	// GameplayCueTag는 색상별 BP 서브클래스가 각자 지정한다.
	bAutoDestroyOnRemove = true;
}

bool AWxCueNotify_AttackTelegraph::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
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

	USceneComponent* AttachTarget = MyTarget->GetRootComponent();
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget))
	{
		AttachTarget = TargetCharacter->GetMesh();
	}

	SpawnedNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachTarget,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy*/ true,
		/*bAutoActivate*/ true);

	return true;
}

bool AWxCueNotify_AttackTelegraph::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);

	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->DeactivateImmediate();
		SpawnedNiagaraComponent = nullptr;
	}

	return true;
}
