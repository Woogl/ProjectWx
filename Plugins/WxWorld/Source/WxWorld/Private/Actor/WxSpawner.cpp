// Copyright Woogle. All Rights Reserved.

#include "Actor/WxSpawner.h"

#include "Actor/WxSpawnableInterface.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "System/WxWorldDeveloperSettings.h"

namespace
{
	constexpr const TCHAR* DefaultSpawnerSpritePath = TEXT("/Engine/EditorResources/Spawn_Point.Spawn_Point");
}

AWxSpawner::AWxSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpriteComponent"));
	SpriteComponent->bIsEditorOnly = true;
	SpriteComponent->SetupAttachment(SceneRoot);
	SpriteComponent->SetRelativeRotation(FRotator(0.f, 0.f, 50.f));

	static ConstructorHelpers::FObjectFinder<UTexture2D> SpriteTexture(DefaultSpawnerSpritePath);
	if (SpriteTexture.Succeeded())
	{
		SpriteComponent->Sprite = SpriteTexture.Object;
	}

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->bIsEditorOnly = true;
	ArrowComponent->SetupAttachment(SceneRoot);
	ArrowComponent->ArrowColor = FColor(150, 200, 255);
	ArrowComponent->ArrowSize = 1.0f;
	ArrowComponent->bTreatAsASprite = true;

	PreviewSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewSkeletalMeshComponent"));
	PreviewSkeletalMeshComponent->bIsEditorOnly = true;
	PreviewSkeletalMeshComponent->SetupAttachment(SceneRoot);
	PreviewSkeletalMeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	PreviewSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewSkeletalMeshComponent->SetHiddenInGame(true);

	PreviewStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewStaticMeshComponent"));
	PreviewStaticMeshComponent->bIsEditorOnly = true;
	PreviewStaticMeshComponent->SetupAttachment(SceneRoot);
	PreviewStaticMeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	PreviewStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewStaticMeshComponent->SetHiddenInGame(true);
#endif
}

void AWxSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !SpawnableActorClass)
	{
		return;
	}

	if (!SpawnableActorClass->ImplementsInterface(UWxSpawnableInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("AWxSpawner: %s does not implement IWxSpawnableInterface and will not be spawned."), *SpawnableActorClass->GetName());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnableActorClass, GetActorLocation(), GetActorRotation(), SpawnParams);
}

void AWxSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SpawnedActor.Reset();

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AWxSpawner::PostLoad()
{
	Super::PostLoad();

	UpdateEditorPreviewFromSpawnableClass();
}

void AWxSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AWxSpawner, SpawnableActorClass))
	{
		UpdateEditorPreviewFromSpawnableClass();
	}
}

void AWxSpawner::UpdateEditorPreviewFromSpawnableClass()
{
	UStreamableRenderAsset* PreviewMesh = nullptr;
	if (SpawnableActorClass)
	{
		if (const IWxSpawnableInterface* Spawnable = Cast<IWxSpawnableInterface>(SpawnableActorClass->GetDefaultObject()))
		{
			PreviewMesh = Spawnable->GetEditorPreviewMesh();
		}
	}

	USkeletalMesh* PreviewSkeletalMesh = Cast<USkeletalMesh>(PreviewMesh);
	UStaticMesh* PreviewStaticMesh = Cast<UStaticMesh>(PreviewMesh);

	if (PreviewSkeletalMeshComponent)
	{
		PreviewSkeletalMeshComponent->SetSkeletalMeshAsset(PreviewSkeletalMesh);
	}

	if (PreviewStaticMeshComponent)
	{
		PreviewStaticMeshComponent->SetStaticMesh(PreviewStaticMesh);
	}

	if (SpriteComponent)
	{
		UTexture2D* NewSprite = GetDefault<UWxWorldDeveloperSettings>()->FindSpawnerIconForClass(SpawnableActorClass);
		if (!NewSprite)
		{
			NewSprite = LoadObject<UTexture2D>(nullptr, DefaultSpawnerSpritePath);
		}
		SpriteComponent->SetSprite(NewSprite);
		
		float MeshTopZ = 0.f;
		if (PreviewSkeletalMesh)
		{
			const FBoxSphereBounds Bounds = PreviewSkeletalMesh->GetBounds();
			MeshTopZ = FMath::Max(MeshTopZ, Bounds.Origin.Z + Bounds.BoxExtent.Z);
		}
		if (PreviewStaticMesh)
		{
			const FBoxSphereBounds Bounds = PreviewStaticMesh->GetBounds();
			MeshTopZ = FMath::Max(MeshTopZ, Bounds.Origin.Z + Bounds.BoxExtent.Z);
		}
		SpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, MeshTopZ + 50.f));
	}
}
#endif
