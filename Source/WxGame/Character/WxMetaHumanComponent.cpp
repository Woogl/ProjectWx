// Copyright Woogle. All Rights Reserved.

#include "Character/WxMetaHumanComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/LODSyncComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GroomComponent.h"

void UWxMetaHumanComponent::OnRegister()
{
	Super::OnRegister();

	// 커맨드릿(에셋 덤프 등)에선 시각 부착물이 불필요하므로 조립하지 않는다.
	if (IsRunningCommandlet())
	{
		return;
	}

	// 데디케이티드 서버도 그릴 대상이 없다. 판정·물리·소켓은 전부 오너 메시가 맡으므로 부착물이 없어도 게임플레이는 그대로다.
	const UWorld* World = GetWorld();
	if (World && World->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	// BP 리컴파일·레벨 스트리밍으로 등록이 반복돼도 조립은 한 번만 한다. (해제 시 전부 파괴되므로 포인터가 곧 가드)
	if (BodyComponent || FaceComponent || OutfitComponent || LODSyncComponent || GroomComponents.Num() > 0)
	{
		return;
	}

	// 오너 캐릭터의 메시가 포즈를 만드는 리더다.
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* LeaderMesh = Owner ? Owner->GetMesh() : nullptr;
	if (!LeaderMesh)
	{
		return;
	}

	if (BodyMesh)
	{
		BodyComponent = CreateAttachedMesh(BodyMesh, TEXT("Body"), LeaderMesh);
		BodyComponent->SetLeaderPoseComponent(LeaderMesh);
		// 숨긴 리더는 렌더러 피드백이 없어 LOD 0에 머문다. 그걸 물려받으면 바디도 0에 묶이므로 자기 화면 크기로 정하게 둔다.
		// 리더가 항상 가장 상세한 LOD라는 성질에 기대는 설정이다 — 리더 LOD를 강제하게 되면 함께 재검토해야 한다.
		BodyComponent->bIgnoreLeaderPoseComponentLOD = true;

		// 표시를 바디가 넘겨받았으므로 리더는 포즈만 만든다. 물리·피격 판정·무기 소켓은 그대로 리더에 남는다.
		LeaderMesh->SetVisibility(false);
	}

	if (FaceMesh)
	{
		FaceComponent = CreateAttachedMesh(FaceMesh, TEXT("Face"), LeaderMesh);
		FaceComponent->SetAnimInstanceClass(FaceAnimClass);
		FaceComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

		// 그룸은 페이스 스켈레톤에 바인딩되므로 페이스가 있을 때만 의미가 있다.
		CreateGroom(Hair, TEXT("Hair"), FaceComponent);
		CreateGroom(Eyebrows, TEXT("Eyebrows"), FaceComponent);
		CreateGroom(Eyelashes, TEXT("Eyelashes"), FaceComponent);
		CreateGroom(Mustache, TEXT("Mustache"), FaceComponent);
		CreateGroom(Beard, TEXT("Beard"), FaceComponent);
	}

	if (OutfitMesh)
	{
		OutfitComponent = CreateAttachedMesh(OutfitMesh, TEXT("Outfit"), LeaderMesh);
		OutfitComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

		// 어셈블 BP의 EnableMasterPose 판정과 동일: 스스로 포즈를 만들 수단(PP-ABP·AnimClass)이 없는 메시만 리더를 따라간다.
		if (!OutfitMesh->GetPostProcessAnimBlueprint() && !OutfitComponent->GetAnimClass())
		{
			OutfitComponent->SetLeaderPoseComponent(LeaderMesh);
		}
	}

	// 엔진 메타휴먼 구현은 이름 문자열로 대상을 찾으므로 실제 생성된 이름을 그대로 채운다. (바디가 없으면 오너 메시가 곧 메타휴먼 바디다)
	// 이 이름으로 페이스 리그로직·넥 보정이 구동된다. 바디 보정은 팔로워가 자기 그래프를 돌리지 않아 평가되지 않는다.
	BodyComponentName = BodyComponent ? BodyComponent->GetName() : LeaderMesh->GetName();
	FaceComponentName = FaceComponent ? FaceComponent->GetName() : FString();

	if (!FaceComponent && !OutfitComponent)
	{
		return;
	}

	// 어셈블 BP는 이름과 매핑을 하드코딩하지만, 여기선 실제 생성된 이름과 각 메시의 LOD 수로 구성한다.
	// 오너 메시는 구동·리더 포즈 전용이라 동기 대상이 아니고, 여기서 만든 표시용 메시끼리만 묶는다.
	const int32 BodyLODCount = BodyMesh ? BodyMesh->GetLODNum() : 0;
	const int32 FaceLODCount = FaceMesh ? FaceMesh->GetLODNum() : 0;
	const int32 OutfitLODCount = OutfitMesh ? OutfitMesh->GetLODNum() : 0;
	const int32 NumSyncLODs = FMath::Max3(BodyLODCount, FaceLODCount, OutfitLODCount);

	LODSyncComponent = NewObject<ULODSyncComponent>(Owner, MakeUniqueObjectName(Owner, ULODSyncComponent::StaticClass(), TEXT("LODSync")), RF_Transient);
	LODSyncComponent->CreationMethod = CreationMethod;
	LODSyncComponent->NumLODs = NumSyncLODs;

	if (BodyComponent)
	{
		AddLODSync(BodyComponent->GetFName(), ESyncOption::Drive, BodyLODCount, NumSyncLODs);
	}

	if (FaceComponent)
	{
		AddLODSync(FaceComponent->GetFName(), ESyncOption::Drive, FaceLODCount, NumSyncLODs);
	}

	if (OutfitComponent)
	{
		AddLODSync(OutfitComponent->GetFName(), ESyncOption::Passive, OutfitLODCount, NumSyncLODs);
	}

	for (UGroomComponent* GroomComponent : GroomComponents)
	{
		AddLODSync(GroomComponent->GetFName(), ESyncOption::Passive, 0, NumSyncLODs);
	}

	LODSyncComponent->RegisterComponent();
}

void UWxMetaHumanComponent::OnUnregister()
{
	// 바디를 만들었다는 것은 우리가 오너 메시를 숨겼다는 뜻이다 — 부착물을 걷어내면서 표시 책임도 돌려준다.
	if (BodyComponent)
	{
		const ACharacter* Owner = Cast<ACharacter>(GetOwner());
		if (USkeletalMeshComponent* LeaderMesh = Owner ? Owner->GetMesh() : nullptr)
		{
			LeaderMesh->SetVisibility(true);
		}
	}

	if (LODSyncComponent)
	{
		LODSyncComponent->DestroyComponent();
		LODSyncComponent = nullptr;
	}
	for (UGroomComponent* GroomComponent : GroomComponents)
	{
		if (GroomComponent)
		{
			GroomComponent->DestroyComponent();
		}
	}
	GroomComponents.Empty();
	if (OutfitComponent)
	{
		OutfitComponent->DestroyComponent();
		OutfitComponent = nullptr;
	}
	if (FaceComponent)
	{
		FaceComponent->DestroyComponent();
		FaceComponent = nullptr;
	}
	if (BodyComponent)
	{
		BodyComponent->DestroyComponent();
		BodyComponent = nullptr;
	}

	Super::OnUnregister();
}

USkeletalMeshComponent* UWxMetaHumanComponent::CreateAttachedMesh(USkeletalMesh* Mesh, FName BaseName, USkeletalMeshComponent* AttachTarget)
{
	AActor* Owner = GetOwner();
	USkeletalMeshComponent* MeshComponent = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), BaseName), RF_Transient);
	// 부착물이 생성 주체와 같은 수명 분류로 취급되도록 CreationMethod를 물려준다.
	MeshComponent->CreationMethod = CreationMethod;
	MeshComponent->SetSkeletalMeshAsset(Mesh);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	MeshComponent->SetupAttachment(AttachTarget);
	MeshComponent->RegisterComponent();

	return MeshComponent;
}

void UWxMetaHumanComponent::CreateGroom(const FWxGroomSlot& Slot, FName BaseName, USkeletalMeshComponent* AttachTarget)
{
	if (!Slot.Groom)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UGroomComponent* GroomComponent = NewObject<UGroomComponent>(Owner, MakeUniqueObjectName(Owner, UGroomComponent::StaticClass(), BaseName), RF_Transient);
	GroomComponent->CreationMethod = CreationMethod;
	GroomComponent->SetGroomAsset(Slot.Groom, Slot.Binding);
	GroomComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// 메타휴먼 그룸의 표준 부착점.
	GroomComponent->AttachmentName = TEXT("FACIAL_C_FacialRoot");
	GroomComponent->SetupAttachment(AttachTarget);
	GroomComponent->RegisterComponent();

	GroomComponents.Add(GroomComponent);
}

void UWxMetaHumanComponent::AddLODSync(FName ComponentName, ESyncOption SyncOption, int32 LODCount, int32 NumSyncLODs)
{
	FComponentSync& Sync = LODSyncComponent->ComponentsToSync.AddDefaulted_GetRef();
	Sync.Name = ComponentName;
	Sync.SyncOption = SyncOption;

	// LOD 수가 모자란 메시는 동기 LOD를 자기 범위로 접어 넣는다.
	if (LODCount > 0 && LODCount < NumSyncLODs)
	{
		FLODMappingData& Mapping = LODSyncComponent->CustomLODMapping.Add(ComponentName);
		for (int32 Index = 0; Index < NumSyncLODs; ++Index)
		{
			Mapping.Mapping.Add(Index * LODCount / NumSyncLODs);
		}
	}
}
