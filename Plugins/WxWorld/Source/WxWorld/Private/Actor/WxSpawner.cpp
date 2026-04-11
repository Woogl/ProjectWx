// Copyright Woogle. All Rights Reserved.

#include "Actor/WxSpawner.h"

#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "System/WxWorldDeveloperSettings.h"

AWxSpawner::AWxSpawner()
{
#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpriteComponent"));
	SpriteComponent->bIsEditorOnly = true;
	SetRootComponent(SpriteComponent);

	static ConstructorHelpers::FObjectFinder<UTexture2D> SpriteTexture(TEXT("/Engine/EditorResources/Spawn_Point.Spawn_Point"));
	if (SpriteTexture.Succeeded())
	{
		SpriteComponent->Sprite = SpriteTexture.Object;
	}
#endif
}

void AWxSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnableActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnableActorClass, GetActorLocation(), GetActorRotation(), SpawnParams);
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

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AWxSpawner, SpawnableActorClass))
	{
		UpdateSpriteFromSpawnableClass();
	}
}

void AWxSpawner::UpdateSpriteFromSpawnableClass()
{
	if (!SpriteComponent)
	{
		return;
	}

	UTexture2D* NewSprite = GetDefault<UWxWorldDeveloperSettings>()->FindSpawnerIconForClass(SpawnableActorClass);
	if (!NewSprite)
	{
		NewSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/Spawn_Point.Spawn_Point"));
	}

	SpriteComponent->SetSprite(NewSprite);
}
#endif
