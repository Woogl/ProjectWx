// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/LODSyncComponent.h"
#include "MetaHumanComponentUE.h"
#include "Templates/SubclassOf.h"
#include "WxMetaHumanComponent.generated.h"

class UAnimInstance;
class UGroomAsset;
class UGroomBindingAsset;
class UGroomComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/** 그룸을 비워두면 해당 슬롯은 생성하지 않는다. */
USTRUCT()
struct FWxGroomSlot
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UGroomAsset> Groom;

	/** 페이스 메시에 구운 바인딩. 없으면 부착만 되고 표정·LOD 추종이 어긋날 수 있다. */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UGroomBindingAsset> Binding;
};

/**
 * 메타휴먼 어셈블 산출물(바디·페이스·그룸·복장)을 오너의 스켈레탈 메시에 조립하는 컴포넌트.
 * 캐릭터 BP마다 어셈블 BP 구성을 수작업으로 복제하는 대신, 에셋만 지정하면 등록 시점에 부착물 일체를 생성·배선한다.
 * LODSync·MetaHuman 컴포넌트가 이름 문자열로 대상을 찾는 구조라, 실제 생성된 컴포넌트 이름을 그대로 채워 이름 어긋남을 차단한다.
 * 오너 캐릭터의 메시가 포즈를 만드는 리더이고, 여기서 만드는 메시는 전부 표시 전용이다.
 * 엔진 MetaHuman 컴포넌트를 상속하는 이유는 대상 지목 프로퍼티가 접근자 없는 protected여서다 — 파생만이 채울 수 있고, 그 덕에 페이스 리그로직·보정 구동도 함께 물려받는다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxMetaHumanComponent : public UMetaHumanComponentUE
{
	GENERATED_BODY()

protected:
	//~ Begin UActorComponent
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End UActorComponent
	
	/** 비워두면 바디를 만들지 않는다. 지정하면 표시를 이 메시가 맡고 오너 메시는 숨긴 구동 전용이 된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	TObjectPtr<USkeletalMesh> BodyMesh;

	/** 비워두면 페이스와 그룸을 만들지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	TObjectPtr<USkeletalMesh> FaceMesh;

	/** 페이스에 걸 AnimBP (예: ABP_Face). 페이스 포스트프로세스 ABP는 메시 에셋에 내장돼 있어 별도 지정이 필요 없다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	TSubclassOf<UAnimInstance> FaceAnimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Hair;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Eyebrows;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Eyelashes;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Mustache;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Beard;

	/** 포스트프로세스 ABP가 없는 메시는 리더 포즈로 따라가게 배선한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	TObjectPtr<USkeletalMesh> OutfitMesh;

private:
	USkeletalMeshComponent* CreateAttachedMesh(USkeletalMesh* Mesh, FName BaseName, USkeletalMeshComponent* AttachTarget);

	void CreateGroom(const FWxGroomSlot& Slot, FName BaseName, USkeletalMeshComponent* AttachTarget);

	/** LODCount가 0이면 매핑 없이 항목만 넣는다. */
	void AddLODSync(FName ComponentName, ESyncOption SyncOption, int32 LODCount, int32 NumSyncLODs);

	// 등록 시점에 생성한 부착물들. 재등록 가드이자 OnUnregister 정리 대상이다.
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> BodyComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> FaceComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGroomComponent>> GroomComponents;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> OutfitComponent;

	UPROPERTY(Transient)
	TObjectPtr<ULODSyncComponent> LODSyncComponent;
};
