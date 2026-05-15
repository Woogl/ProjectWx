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

void AWxGimmick::OnWxSaveRestored()
{
	// BeginPlay 이전 복원이면 곧 호출될 BeginPlay 가 ApplyState 를 호출하므로 생략.
	// BeginPlay 이후 복원(스트리밍 인-스트림 등) 이면 즉시 시각/인터랙션 동기화.
	if (HasActorBegunPlay())
	{
		ApplyState();
	}
}

void AWxGimmick::OnRep_bTriggered()
{
	ApplyState();
}
