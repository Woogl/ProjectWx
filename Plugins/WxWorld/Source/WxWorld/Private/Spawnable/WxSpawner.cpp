// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxSpawner.h"

#include "Spawnable/WxSpawnableInterface.h"
#include "System/WxSpawnerSubsystem.h"
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
	PreviewSkeletalMeshComponent->SetupAttachment(SceneRoot);
	PreviewSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewSkeletalMeshComponent->SetHiddenInGame(true);
	PreviewSkeletalMeshComponent->bCastHiddenShadow = true;

	PreviewStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewStaticMeshComponent"));
	PreviewStaticMeshComponent->SetupAttachment(SceneRoot);
	PreviewStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewStaticMeshComponent->SetHiddenInGame(true);
	PreviewStaticMeshComponent->bCastHiddenShadow = true;
#endif
}

void AWxSpawner::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITORONLY_DATA
	if (PreviewSkeletalMeshComponent)
	{
		PreviewSkeletalMeshComponent->DestroyComponent();
		PreviewSkeletalMeshComponent = nullptr;
	}
	if (PreviewStaticMeshComponent)
	{
		PreviewStaticMeshComponent->DestroyComponent();
		PreviewStaticMeshComponent = nullptr;
	}
#endif

	if (HasAuthority())
	{
		if (UWxSpawnerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWxSpawnerSubsystem>())
		{
			Subsystem->RegisterSpawner(this);

			if (Subsystem->IsSpawnerKilled(this))
			{
				return;
			}
		}
	}

	SpawnTarget();
}

void AWxSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UWxSpawnerSubsystem* Subsystem = World->GetSubsystem<UWxSpawnerSubsystem>())
			{
				Subsystem->UnregisterSpawner(this);
			}
		}

		if (AActor* Existing = SpawnedActor.Get())
		{
			Existing->OnDestroyed.RemoveDynamic(this, &AWxSpawner::HandleSpawnedActorDestroyed);
			Existing->Destroy();
		}
	}

	SpawnedActor.Reset();

	Super::EndPlay(EndPlayReason);
}

void AWxSpawner::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AActor* Existing = SpawnedActor.Get())
	{
		Existing->OnDestroyed.RemoveDynamic(this, &AWxSpawner::HandleSpawnedActorDestroyed);
		Existing->Destroy();
	}
	SpawnedActor.Reset();

	SpawnTarget();
}

void AWxSpawner::HandleSpawnedActorDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UWxSpawnerSubsystem* Subsystem = World->GetSubsystem<UWxSpawnerSubsystem>())
	{
		Subsystem->MarkSpawnerKilled(this);
	}

	SpawnedActor.Reset();
}

void AWxSpawner::SpawnTarget()
{
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
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnableActorClass, GetActorLocation(), GetActorRotation(), SpawnParams);

	if (AActor* Spawned = SpawnedActor.Get())
	{
		Spawned->OnDestroyed.AddDynamic(this, &AWxSpawner::HandleSpawnedActorDestroyed);
	}
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
	const UMeshComponent* SourceMeshComponent = nullptr;
	if (SpawnableActorClass)
	{
		if (const IWxSpawnableInterface* Spawnable = Cast<IWxSpawnableInterface>(SpawnableActorClass->GetDefaultObject()))
		{
			SourceMeshComponent = Spawnable->GetEditorPreviewMeshComponent();
		}
	}

	const USkeletalMeshComponent* SourceSkeletal = Cast<USkeletalMeshComponent>(SourceMeshComponent);
	const UStaticMeshComponent* SourceStatic = Cast<UStaticMeshComponent>(SourceMeshComponent);

	USkeletalMesh* PreviewSkeletalMesh = SourceSkeletal ? SourceSkeletal->GetSkeletalMeshAsset() : nullptr;
	UStaticMesh* PreviewStaticMesh = SourceStatic ? SourceStatic->GetStaticMesh() : nullptr;

	// 루트 프리미티브의 로컬 바운드 하단만큼 올려 발이 스포너 원점에 맞도록 보정
	// (캡슐 → HalfHeight, 박스 → ExtentZ, 스피어 → Radius, 메시 → 메시 바운드 하단)
	FVector GroundOffset = FVector::ZeroVector;
	if (const AActor* SourceCDO = SpawnableActorClass ? SpawnableActorClass->GetDefaultObject<AActor>() : nullptr)
	{
		if (const UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(SourceCDO->GetRootComponent()))
		{
			const FBoxSphereBounds LocalBounds = RootPrim->CalcBounds(FTransform::Identity);
			GroundOffset.Z = FMath::Max(0.f, -LocalBounds.GetBox().Min.Z);
		}
	}

	if (PreviewSkeletalMeshComponent)
	{
		PreviewSkeletalMeshComponent->SetSkeletalMeshAsset(PreviewSkeletalMesh);
		PreviewSkeletalMeshComponent->EmptyOverrideMaterials();

		FTransform SkeletalTransform = SourceSkeletal ? SourceSkeletal->GetRelativeTransform() : FTransform::Identity;
		SkeletalTransform.AddToTranslation(GroundOffset);
		PreviewSkeletalMeshComponent->SetRelativeTransform(SkeletalTransform);

		if (SourceSkeletal)
		{
			const int32 NumMaterials = SourceSkeletal->GetNumMaterials();
			for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
			{
				PreviewSkeletalMeshComponent->SetMaterial(MaterialIndex, SourceSkeletal->GetMaterial(MaterialIndex));
			}
		}
	}

	if (PreviewStaticMeshComponent)
	{
		PreviewStaticMeshComponent->SetStaticMesh(PreviewStaticMesh);
		PreviewStaticMeshComponent->EmptyOverrideMaterials();

		FTransform StaticTransform = SourceStatic ? SourceStatic->GetRelativeTransform() : FTransform::Identity;
		StaticTransform.AddToTranslation(GroundOffset);
		PreviewStaticMeshComponent->SetRelativeTransform(StaticTransform);

		if (SourceStatic)
		{
			const int32 NumMaterials = SourceStatic->GetNumMaterials();
			for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
			{
				PreviewStaticMeshComponent->SetMaterial(MaterialIndex, SourceStatic->GetMaterial(MaterialIndex));
			}
		}
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

	if (SpawnableActorClass)
	{
		FString NewLabel = SpawnableActorClass->GetName();
		NewLabel.RemoveFromEnd(TEXT("_C"));
		SetActorLabel(NewLabel);
	}
	else
	{
		SetActorLabel(GetClass()->GetName());
	}
}
#endif
