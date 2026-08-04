// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxSpawner.h"

#include "Spawnable/WxSpawnable.h"
#include "Components/BillboardComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "System/WxWorldDeveloperSettings.h"
#include "WxWorldModule.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

namespace WxSpawnerLabel
{
	/** 자동 생성 라벨의 접두사. 아웃라이너 정렬에서 스포너를 한 덩어리로 모으고, 디자이너가 지은 이름과 구분하는 표식도 겸한다. */
	static const TCHAR* Prefix = TEXT("Spawner");
}

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

	// 프리뷰 컴포넌트는 여기서 만들지 않는다. CDO 서브오브젝트로 두면 게임 월드에도 딸려오고 RF_Transient 도 붙지 않으므로,
	// 에디터 월드에서만 PostRegisterAllComponents 가 NewObject 로 생성한다.
#endif
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

EWxSpawnerMode AWxSpawner::GetSpawnMode() const
{
	return SpawnMode;
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

FGuid AWxSpawner::GetSaveId() const
{
	return SaveId;
}

void AWxSpawner::OnSaveRestored()
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

void AWxSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 정상 흐름에선 BeginPlay 시점에 bIsKilled=false.
		// 슬롯 복원으로 true 가 들어왔다면 OnSaveRestored 가 SpawnedActor 정리를 담당.
		// 본 가드는 에디터 디폴트 등으로 true 가 들어오는 경우의 안전망.
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

void AWxSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (AActor* Existing = SpawnedActor.Get())
		{
			Existing->Destroy();
		}
	}

	SpawnedActor.Reset();

	Super::EndPlay(EndPlayReason);
}

void AWxSpawner::SpawnTarget()
{
	if (!HasAuthority() || !SpawnableActorClass)
	{
		return;
	}

	if (!SpawnableActorClass->ImplementsInterface(UWxSpawnable::StaticClass()))
	{
		UE_LOG(LogWxWorld, Warning, TEXT("AWxSpawner: %s does not implement IWxSpawnable and will not be spawned."), *SpawnableActorClass->GetName());
		return;
	}

	// Deferred Spawn 으로 빙의(AutoPossessAI) 전에 OnSpawnedBy 컨텍스트를 주입한다.
	// 일반 SpawnActor 는 빙의가 호출 내부에서 끝나므로, 그 뒤엔 컨트롤러 OnPossess 가 컨텍스트를 보지 못한다.
	const FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
	AActor* Spawned = GetWorld()->SpawnActorDeferred<AActor>(
		SpawnableActorClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Spawned)
	{
		return;
	}

	if (IWxSpawnable* Spawnable = Cast<IWxSpawnable>(Spawned))
	{
		Spawnable->OnSpawnedBy(this);
	}

	Spawned->FinishSpawning(SpawnTransform);

	// 스포너에 attach 하지 않는다. 스폰 대상은 CMC 로 돌아다니는 캐릭터라, 루트가 붙어 있으면 이동 복제가 ReplicatedMovement 대신 AttachmentReplication(부모 상대 오프셋) 경로를 타 원격 스무딩에서 벗어나고 스포너를 옮기면 딸려 온다.
	// 수명은 이 약참조와 Respawn/EndPlay/OnSaveRestored 의 명시 Destroy 가 관리하므로 부착이 필요 없다.
	SpawnedActor = Spawned;
}

#if WITH_EDITOR
// 에디터 전용 GetActorGuid() 를 런타임 가용 UPROPERTY 로 복사한다.
// ActorGuid 는 에디터에서 액터별로 안정·고유하고, 부여된 WxSaveId 는 에셋 저장 시 직렬화되어 쿠커가 그대로 읽으므로 런타임 키가 보장된다.
void AWxSpawner::PostActorCreated()
{
	Super::PostActorCreated();

	SaveId = GetActorGuid();
}

void AWxSpawner::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// 복제 시 엔진이 새 ActorGuid 를 부여하므로 그대로 따라가면 원본과 충돌하지 않는다.
	SaveId = GetActorGuid();
}

// 프리뷰 컴포넌트를 에디터 월드에서만 만들어 게임 월드로 새지 않게 한다.
// 생성 훅으로 PreRegisterAllComponents 는 쓸 수 없다. 월드파티션 셀 스트리밍이 타는 증분 등록 경로가 그 함수를 호출하지 않는다.
void AWxSpawner::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	const UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}

	if (!PreviewChildActorComponent)
	{
		// RF_Transient 는 자식 액터까지 전파되어, 자식이 스포너의 외부 패키지에 얹히는 것을 막는다.
		PreviewChildActorComponent = NewObject<UChildActorComponent>(this, TEXT("PreviewChildActorComponent"), RF_Transient);
		PreviewChildActorComponent->SetupAttachment(SceneRoot);

		// 자식 액터에 bIsEditorOnlyActor 를 세우고, 아웃라이너에서 감춘다.
		PreviewChildActorComponent->SetIsVisualizationComponent(true);
		PreviewChildActorComponent->SetEditorTreeViewVisualizationMode(EChildActorComponentTreeViewVisualizationMode::Hidden);

		PreviewChildActorComponent->RegisterComponent();
	}

	UpdateEditorPreviewFromSpawnableClass();
}

void AWxSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AWxSpawner, SpawnableActorClass))
	{
		UpdateEditorPreviewFromSpawnableClass();

		// 라벨 동기화는 프리뷰 갱신과 달리 여기(디자이너가 클래스를 실제로 바꾼 순간)에서만 한다.
		// 프리뷰 쪽에 두면 맵/셀을 열 때마다 도는 PostRegisterAllComponents 경로에서도 불려, 디자이너가 지은 이름(예: Boss_Room_Guard_01)이 매 로드마다 클래스명으로 되돌아가고 SetActorLabel 의 Modify() 가 아무 편집 없이 패키지를 dirty 로 만든다.
		// 접두사로 시작하는 자동 라벨일 때만 갱신해, 손수 지은 이름은 대상 클래스를 바꿔도 남긴다.
		if (GetActorLabel().StartsWith(WxSpawnerLabel::Prefix))
		{
			// 엔진이 액터를 배치할 때와 같은 경로다. 중복 라벨에 번호를 붙여 같은 클래스를 여러 기 놓아도 Spawner_Enemy2, Spawner_Enemy3 으로 갈린다.
			FActorLabelUtilities::SetActorLabelUnique(this, GetDefaultActorLabel());
		}
	}
}

FString AWxSpawner::GetDefaultActorLabel() const
{
	if (!SpawnableActorClass)
	{
		return WxSpawnerLabel::Prefix;
	}

	// 블루프린트 클래스의 _C 접미만 떼어 에셋 이름 그대로 붙인다. BP_Enemy 는 Spawner_BP_Enemy 가 된다.
	FString TargetName = SpawnableActorClass->GetName();
	TargetName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);

	return FString::Printf(TEXT("%s_%s"), WxSpawnerLabel::Prefix, *TargetName);
}

void AWxSpawner::UpdateEditorPreviewFromSpawnableClass()
{
	// 프리뷰 루트의 상단 높이. 스프라이트를 그 위에 띄우는 데 쓴다.
	float PreviewTopZ = 0.f;

	if (PreviewChildActorComponent)
	{
		// 등록된 컴포넌트에 클래스를 지정하면 엔진이 자식 액터를 즉시 재생성한다. 부착물·머티리얼·소켓 배치가 함께 따라온다.
		PreviewChildActorComponent->SetChildActorClass(SpawnableActorClass);

		// 캐릭터는 캡슐 중심이 액터 원점이라 그대로 두면 허리까지 묻힌다. 루트 바운드 하단만큼 올려 발을 스포너 원점에 맞춘다.
		// 바운드는 컴포넌트의 현재 위치와 무관한 로컬 값이므로 이 계산은 몇 번을 호출해도 같은 결과가 나온다.
		float FootOffset = 0.f;
		if (const AActor* PreviewActor = PreviewChildActorComponent->GetChildActor())
		{
			if (const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(PreviewActor->GetRootComponent()))
			{
				const FTransform ScaleOnly(FQuat::Identity, FVector::ZeroVector, Root->GetRelativeScale3D());
				const FBox RootBounds = Root->CalcBounds(ScaleOnly).GetBox();

				FootOffset = FMath::Max(0.f, -RootBounds.Min.Z);
				PreviewTopZ = FootOffset + RootBounds.Max.Z;
			}
		}
		PreviewChildActorComponent->SetRelativeLocation(FVector(0.f, 0.f, FootOffset));
	}

	if (SpriteComponent)
	{
		UTexture2D* NewSprite = GetDefault<UWxWorldDeveloperSettings>()->FindSpawnerIconForClass(SpawnableActorClass);
		if (!NewSprite)
		{
			NewSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/Spawn_Point.Spawn_Point"));
		}
		SpriteComponent->SetSprite(NewSprite);
		SpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, PreviewTopZ + 50.f));
	}
}
#endif
