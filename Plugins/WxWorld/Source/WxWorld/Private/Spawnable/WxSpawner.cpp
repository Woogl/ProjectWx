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

AWxSpawner::AWxSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("SpriteComponent"));
	if (SpriteComponent)
	{
		SpriteComponent->SetupAttachment(SceneRoot);
		SpriteComponent->SetRelativeRotation(FRotator(0.f, 0.f, 50.f));

		static ConstructorHelpers::FObjectFinder<UTexture2D> SpriteTexture(TEXT("/Engine/EditorResources/Spawn_Point.Spawn_Point"));
		if (SpriteTexture.Succeeded())
		{
			SpriteComponent->Sprite = SpriteTexture.Object;
		}
	}

	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->bTreatAsASprite = true;
	}

	PreviewSkeletalMeshComponent = CreateEditorOnlyDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewSkeletalMeshComponent"));
	if (PreviewSkeletalMeshComponent)
	{
		PreviewSkeletalMeshComponent->SetupAttachment(SceneRoot);
		PreviewSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewSkeletalMeshComponent->SetHiddenInGame(true);
		PreviewSkeletalMeshComponent->bCastHiddenShadow = true;
	}

	PreviewStaticMeshComponent = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewStaticMeshComponent"));
	if (PreviewStaticMeshComponent)
	{
		PreviewStaticMeshComponent->SetupAttachment(SceneRoot);
		PreviewStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewStaticMeshComponent->SetHiddenInGame(true);
		PreviewStaticMeshComponent->bCastHiddenShadow = true;
	}
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
		}

		// 정상 흐름에선 BeginPlay 시점에 bIsKilled=false. 슬롯 복원으로 true 가 들어왔다면
		// OnWxSaveRestored 가 SpawnedActor 정리를 담당. 본 가드는 에디터 디폴트 등으로 true 가 들어오는 경우의 안전망.
		if (bIsKilled)
		{
			return;
		}
	}

	if (SpawnMode == EWxSpawnerMode::Auto)
	{
		SpawnTarget();
	}
}

void AWxSpawner::OnWxSaveRestored()
{
	// 슬롯 복원으로 bIsKilled=true 가 적용되었지만 BeginPlay 가 먼저 spawn 한 인스턴스가 남아있다면 정리.
	if (bIsKilled)
	{
		if (AActor* Existing = SpawnedActor.Get())
		{
			Existing->Destroy();
		}
		SpawnedActor.Reset();
	}
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
			Existing->Destroy();
		}
	}

	SpawnedActor.Reset();

	Super::EndPlay(EndPlayReason);
}

EWxSpawnerMode AWxSpawner::GetSpawnMode() const
{
	return SpawnMode;
}

AActor* AWxSpawner::GetSpawnedActor() const
{
	return SpawnedActor.Get();
}

bool AWxSpawner::IsKilled() const
{
	return bIsKilled;
}

void AWxSpawner::MarkKilled()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsKilled = true;
}

void AWxSpawner::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	// 기존 액터 정리. 시체면 청소, 살아있으면 위치/상태 원복을 위한 destroy.
	if (AActor* Existing = SpawnedActor.Get())
	{
		Existing->Destroy();
	}
	SpawnedActor.Reset();

	// 부활 금지 Spawner (보스 등) 는 죽은 뒤 새 인스턴스를 spawn 하지 않는다.
	if (bIsKilled && bNeverRevive)
	{
		return;
	}

	// 부활 가능 Spawner 는 처치 기록 리셋 후 새 인스턴스 생성.
	bIsKilled = false;
	SpawnTarget();
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
		Spawned->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
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

	// 소스 메시의 액터 기준 누적 Transform (상위 체인의 Location/Rotation/Scale 반영)
	FTransform PreviewTransform = FTransform::Identity;
	for (const USceneComponent* C = SourceMeshComponent; C; C = C->GetAttachParent())
	{
		PreviewTransform = PreviewTransform * C->GetRelativeTransform();
	}

	// 루트 바운드(Scale 반영) 하단만큼 올려 발을 스포너 원점에 정렬
	if (const AActor* CDO = SpawnableActorClass ? SpawnableActorClass->GetDefaultObject<AActor>() : nullptr)
	{
		if (const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(CDO->GetRootComponent()))
		{
			const FTransform ScaleOnly(FQuat::Identity, FVector::ZeroVector, Root->GetRelativeScale3D());
			PreviewTransform.AddToTranslation(FVector(0.f, 0.f, FMath::Max(0.f, -Root->CalcBounds(ScaleOnly).GetBox().Min.Z)));
		}
	}

	if (PreviewSkeletalMeshComponent)
	{
		PreviewSkeletalMeshComponent->SetSkeletalMeshAsset(SourceSkeletal ? SourceSkeletal->GetSkeletalMeshAsset() : nullptr);
		PreviewSkeletalMeshComponent->EmptyOverrideMaterials();
		PreviewSkeletalMeshComponent->SetRelativeTransform(PreviewTransform);
		if (SourceSkeletal)
		{
			for (int32 i = 0; i < SourceSkeletal->GetNumMaterials(); ++i)
			{
				PreviewSkeletalMeshComponent->SetMaterial(i, SourceSkeletal->GetMaterial(i));
			}
		}
	}

	if (PreviewStaticMeshComponent)
	{
		PreviewStaticMeshComponent->SetStaticMesh(SourceStatic ? SourceStatic->GetStaticMesh() : nullptr);
		PreviewStaticMeshComponent->EmptyOverrideMaterials();
		PreviewStaticMeshComponent->SetRelativeTransform(PreviewTransform);
		if (SourceStatic)
		{
			for (int32 i = 0; i < SourceStatic->GetNumMaterials(); ++i)
			{
				PreviewStaticMeshComponent->SetMaterial(i, SourceStatic->GetMaterial(i));
			}
		}
	}

	if (SpriteComponent)
	{
		UTexture2D* NewSprite = GetDefault<UWxWorldDeveloperSettings>()->FindSpawnerIconForClass(SpawnableActorClass);
		if (!NewSprite)
		{
			NewSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/Spawn_Point.Spawn_Point"));
		}
		SpriteComponent->SetSprite(NewSprite);

		const UPrimitiveComponent* ActivePreview = SourceSkeletal ? static_cast<UPrimitiveComponent*>(PreviewSkeletalMeshComponent)
			: SourceStatic ? static_cast<UPrimitiveComponent*>(PreviewStaticMeshComponent) : nullptr;
		const float TopZ = ActivePreview ? ActivePreview->CalcBounds(PreviewTransform).GetBox().Max.Z : 0.f;
		SpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, TopZ + 50.f));
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
