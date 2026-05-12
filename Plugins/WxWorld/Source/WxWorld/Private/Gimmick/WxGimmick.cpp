// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Components/ArrowComponent.h"
#include "Net/UnrealNetwork.h"

AWxGimmick::AWxGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->ArrowColor = FColor(255, 200, 0);
		ArrowComponent->bTreatAsASprite = true;
	}
#endif
}

void AWxGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxGimmick, bTriggered);
}

void AWxGimmick::MarkTriggered()
{
	if (!HasAuthority() || bTriggered)
	{
		return;
	}

	bTriggered = true;
	ApplyState();
}

void AWxGimmick::OnRep_bTriggered()
{
	ApplyState();
}
