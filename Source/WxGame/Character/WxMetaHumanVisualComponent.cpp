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

	// BP 리컴파일·레벨 스트리밍으로 등록이 반복돼도 조립은 한 번만 한다. (해제 시 전부 파괴되므로 포인터가 곧 가드)
	if (FaceComponent || OutfitComponent || LODSyncComponent || MetaHumanComponent || GroomComponents.Num() > 0)
	{
		return;
	}

	USkeletalMeshComponent* BodyMesh = ResolveBodyMesh();
	if (!BodyMesh)
	{
		return;
	}

	AActor* Owner = GetOwner();

	if (FaceMesh)
	{
		FaceComponent = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("Face")), RF_Transient);
		FaceComponent->SetSkeletalMeshAsset(FaceMesh);
		FaceComponent->SetAnimInstanceClass(FaceAnimClass);
		FaceComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		FaceComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		FaceComponent->SetupAttachment(BodyMesh);
		FaceComponent->RegisterComponent();

		// 그룸은 페이스 스켈레톤에 바인딩되므로 페이스가 있을 때만 의미가 있다.
		CreateGroom(Hair, TEXT("Hair"), FaceComponent);
		CreateGroom(Eyebrows, TEXT("Eyebrows"), FaceComponent);
		CreateGroom(Eyelashes, TEXT("Eyelashes"), FaceComponent);
		CreateGroom(Peachfuzz, TEXT("Peachfuzz"), FaceComponent);
		CreateGroom(Mustache, TEXT("Mustache"), FaceComponent);
		CreateGroom(Beard, TEXT("Beard"), FaceComponent);
	}

	if (OutfitMesh)
	{
		OutfitComponent = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("Outfit")), RF_Transient);
		OutfitComponent->SetSkeletalMeshAsset(OutfitMesh);
		OutfitComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		OutfitComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		OutfitComponent->SetupAttachment(BodyMesh);
		OutfitComponent->RegisterComponent();

		// 어셈블 BP의 EnableMasterPose 판정과 동일: 스스로 포즈를 만들 수단(PP-ABP·AnimClass)이 없는 메시만 바디를 따라간다.
		if (!OutfitMesh->GetPostProcessAnimBlueprint() && !OutfitComponent->GetAnimClass())
		{
			OutfitComponent->SetLeaderPoseComponent(BodyMesh);
		}
	}

	if (!FaceComponent && !OutfitComponent)
	{
		return;
	}

	// LOD 동기화: 바디·페이스가 구동하고 나머지는 따라간다.
	// 어셈블 BP는 이름과 매핑을 하드코딩하지만, 여기선 실제 생성된 이름과 각 메시의 LOD 수로 구성한다.
	const int32 BodyLODCount = BodyMesh->GetSkeletalMeshAsset() ? BodyMesh->GetSkeletalMeshAsset()->GetLODNum() : 0;
	const int32 FaceLODCount = FaceMesh ? FaceMesh->GetLODNum() : 0;
	const int32 OutfitLODCount = OutfitMesh ? OutfitMesh->GetLODNum() : 0;
	const int32 NumSyncLODs = FMath::Max3(BodyLODCount, FaceLODCount, OutfitLODCount);

	LODSyncComponent = NewObject<ULODSyncComponent>(Owner, MakeUniqueObjectName(Owner, ULODSyncComponent::StaticClass(), TEXT("LODSync")), RF_Transient);
	LODSyncComponent->NumLODs = NumSyncLODs;

	FComponentSync& BodySync = LODSyncComponent->ComponentsToSync.AddDefaulted_GetRef();
	BodySync.Name = BodyMesh->GetFName();
	BodySync.SyncOption = ESyncOption::Drive;
	if (BodyLODCount > 0 && BodyLODCount < NumSyncLODs)
	{
		FLODMappingData& Mapping = LODSyncComponent->CustomLODMapping.Add(BodyMesh->GetFName());
		for (int32 Index = 0; Index < NumSyncLODs; ++Index)
		{
			Mapping.Mapping.Add(Index * BodyLODCount / NumSyncLODs);
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

	// 페이스 리그로직·넥 보정·바디 보정 구동. 대상 컴포넌트를 이름으로 찾으므로 실제 이름을 넘긴다.
	if (FaceComponent)
	{
		MetaHumanComponent = NewObject<UWxMetaHumanComponent>(Owner, MakeUniqueObjectName(Owner, UWxMetaHumanComponent::StaticClass(), TEXT("MetaHuman")), RF_Transient);
		MetaHumanComponent->SetTargetComponentNames(BodyMesh->GetName(), FaceComponent->GetName());
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

	Super::OnUnregister();
}

USkeletalMeshComponent* UWxMetaHumanVisualComponent::ResolveBodyMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		return Character->GetMesh();
	}
	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}

void UWxMetaHumanVisualComponent::CreateGroom(const FWxGroomSlot& Slot, const TCHAR* BaseName, USkeletalMeshComponent* AttachTarget)
{
	if (!Slot.Groom)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UGroomComponent* GroomComponent = NewObject<UGroomComponent>(Owner, MakeUniqueObjectName(Owner, UGroomComponent::StaticClass(), BaseName), RF_Transient);
	GroomComponent->SetGroomAsset(Slot.Groom, Slot.Binding);
	// 메타휴먼 그룸의 표준 부착점. 페이스 스켈레톤의 얼굴 루트 본에 붙는다.
	GroomComponent->AttachmentName = TEXT("FACIAL_C_FacialRoot");
	GroomComponent->SetupAttachment(AttachTarget);
	GroomComponent->RegisterComponent();

	GroomComponents.Add(GroomComponent);
}
