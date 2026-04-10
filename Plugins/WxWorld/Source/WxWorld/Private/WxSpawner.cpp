// Copyright Woogle. All Rights Reserved.

#include "WxSpawner.h"
#include "Components/ChildActorComponent.h"

AWxSpawner::AWxSpawner()
{
#if WITH_EDITORONLY_DATA
	PreviewChildActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("PreviewChildActor"));
	PreviewChildActor->bIsEditorOnly = true;
	SetRootComponent(PreviewChildActor);
#endif
}

void AWxSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (ActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass, GetActorLocation(), GetActorRotation(), SpawnParams);
	}
}

void AWxSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Actor = SpawnedActor.Get())
	{
		Actor->Destroy();
	}
	SpawnedActor.Reset();

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AWxSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	PreviewChildActor->SetChildActorClass(ActorClass);
}
#endif
