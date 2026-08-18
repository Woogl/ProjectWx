// Copyright Woogle. All Rights Reserved.

#include "Character/WxMetaHumanVisualComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/LODSyncComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GroomComponent.h"

void UWxMetaHumanComponent::SetTargetComponentNames(const FString& InBodyComponentName, const FString& InFaceComponentName)
{
	BodyComponentName = InBodyComponentName;
	FaceComponentName = InFaceComponentName;
}

void UWxMetaHumanVisualComponent::OnRegister()
{
	Super::OnRegister();

	// 커맨드릿(에셋 덤프 등)에선 시각 부착물이 불필요하므로 조립하지 않는다.
	if (IsRunningCommandlet())
	{
		return;
	}

	// BP 리컴파일·레벨 스트리밍으로 등록이 반복돼도 조립은 한 번만 한다. (해제 시 전부 파괴되므로 포인터가 곧 가드)
	if (BodyComponent || FaceComponent || OutfitComponent || LODSyncComponent || MetaHumanComponent || GroomComponents.Num() > 0)
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
		BodyComponent = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("Body")), RF_Transient);
		// 부착물이 생성 주체와 같은 수명 분류로 취급되도록 CreationMethod를 물려준다. (이하 모든 부착물 동일)
		BodyComponent->CreationMethod = CreationMethod;
		BodyComponent->SetSkeletalMeshAsset(BodyMesh);
		BodyComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		BodyComponent->SetupAttachment(LeaderMesh);
		BodyComponent->RegisterComponent();
		BodyComponent->SetLeaderPoseComponent(LeaderMesh);

		// 표시를 바디가 넘겨받았으므로 리더는 포즈만 만든다. 물리·피격 판정·무기 소켓은 그대로 리더에 남는다.
		LeaderMesh->SetVisibility(false);
	}

	if (FaceMesh)
	{
		FaceComponent = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("Face")), RF_Transient);
		FaceComponent->CreationMethod = CreationMethod;
		FaceComponent->SetSkeletalMeshAsset(FaceMesh);
		FaceComponent->SetAnimInstanceClass(FaceAnimClass);
		FaceComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		FaceComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		FaceComponent->SetupAttachment(LeaderMesh);
		FaceComponent->RegisterComponent();

		// 그룸은 페이스 스켈레톤에 바인딩되므로 페이스가 있을 때만 의미가 있다.
		CreateGroom(Hair, TEXT("Hair"), FaceComponent);
		CreateGroom(Eyebrows, TEXT("Eyebrows"), FaceComponent);
		CreateGroom(Eyelashes, TEXT("Eyelashes"), FaceComponent);
		CreateGroom(Mustache, TEXT("Mustache"), FaceComponent);
		CreateGroom(Beard, TEXT("Beard"), FaceComponent);
	}

	if (OutfitMesh)
	{
		OutfitComponent = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("Outfit")), RF_Transient);
		OutfitComponent->CreationMethod = CreationMethod;
		OutfitComponent->SetSkeletalMeshAsset(OutfitMesh);
		OutfitComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		OutfitComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		OutfitComponent->SetupAttachment(LeaderMesh);
		OutfitComponent->RegisterComponent();

		// 어셈블 BP의 EnableMasterPose 판정과 동일: 스스로 포즈를 만들 수단(PP-ABP·AnimClass)이 없는 메시만 리더를 따라간다.
		if (!OutfitMesh->GetPostProcessAnimBlueprint() && !OutfitComponent->GetAnimClass())
		{
			OutfitComponent->SetLeaderPoseComponent(LeaderMesh);
		}
	}

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
		FComponentSync& BodySync = LODSyncComponent->ComponentsToSync.AddDefaulted_GetRef();
		BodySync.Name = BodyComponent->GetFName();
		BodySync.SyncOption = ESyncOption::Drive;
		if (BodyLODCount > 0 && BodyLODCount < NumSyncLODs)
		{
			FLODMappingData& Mapping = LODSyncComponent->CustomLODMapping.Add(BodyComponent->GetFName());
			for (int32 Index = 0; Index < NumSyncLODs; ++Index)
			{
				Mapping.Mapping.Add(Index * BodyLODCount / NumSyncLODs);
			}
		}
	}

	if (FaceComponent)
	{
		FComponentSync& FaceSync = LODSyncComponent->ComponentsToSync.AddDefaulted_GetRef();
		FaceSync.Name = FaceComponent->GetFName();
		FaceSync.SyncOption = ESyncOption::Drive;
	}

	if (OutfitComponent)
	{
		FComponentSync& OutfitSync = LODSyncComponent->ComponentsToSync.AddDefaulted_GetRef();
		OutfitSync.Name = OutfitComponent->GetFName();
		OutfitSync.SyncOption = ESyncOption::Passive;
		if (OutfitLODCount > 0 && OutfitLODCount < NumSyncLODs)
		{
			FLODMappingData& Mapping = LODSyncComponent->CustomLODMapping.Add(OutfitComponent->GetFName());
			for (int32 Index = 0; Index < NumSyncLODs; ++Index)
			{
				Mapping.Mapping.Add(Index * OutfitLODCount / NumSyncLODs);
			}
		}
	}

	for (UGroomComponent* GroomComponent : GroomComponents)
	{
		FComponentSync& GroomSync = LODSyncComponent->ComponentsToSync.AddDefaulted_GetRef();
		GroomSync.Name = GroomComponent->GetFName();
		GroomSync.SyncOption = ESyncOption::Passive;
	}

	LODSyncComponent->RegisterComponent();

	// 페이스 리그로직·넥 보정 구동.
	// 바디 보정은 바디가 리더 포즈 팔로워면 자기 그래프를 돌리지 않아 평가되지 않는다.
	if (FaceComponent)
	{
		MetaHumanComponent = NewObject<UWxMetaHumanComponent>(Owner, MakeUniqueObjectName(Owner, UWxMetaHumanComponent::StaticClass(), TEXT("MetaHuman")), RF_Transient);
		MetaHumanComponent->CreationMethod = CreationMethod;
		MetaHumanComponent->SetTargetComponentNames(BodyComponent ? BodyComponent->GetName() : LeaderMesh->GetName(), FaceComponent->GetName());
		MetaHumanComponent->RegisterComponent();
	}
}

void UWxMetaHumanVisualComponent::OnUnregister()
{
	if (MetaHumanComponent)
	{
		MetaHumanComponent->DestroyComponent();
		MetaHumanComponent = nullptr;
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

void UWxMetaHumanVisualComponent::CreateGroom(const FWxGroomSlot& Slot, const TCHAR* BaseName, USkeletalMeshComponent* AttachTarget)
{
	if (!Slot.Groom)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UGroomComponent* GroomComponent = NewObject<UGroomComponent>(Owner, MakeUniqueObjectName(Owner, UGroomComponent::StaticClass(), BaseName), RF_Transient);
	GroomComponent->CreationMethod = CreationMethod;
	GroomComponent->SetGroomAsset(Slot.Groom, Slot.Binding);
	// 메타휴먼 그룸의 표준 부착점.
	GroomComponent->AttachmentName = TEXT("FACIAL_C_FacialRoot");
	GroomComponent->SetupAttachment(AttachTarget);
	GroomComponent->RegisterComponent();

	GroomComponents.Add(GroomComponent);
}
