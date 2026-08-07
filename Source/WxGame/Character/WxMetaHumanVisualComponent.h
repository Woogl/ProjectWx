// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MetaHumanComponentUE.h"
#include "Templates/SubclassOf.h"
#include "WxMetaHumanVisualComponent.generated.h"

class UAnimInstance;
class UGroomAsset;
class UGroomBindingAsset;
class UGroomComponent;
class ULODSyncComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 엔진 MetaHuman 컴포넌트의 바디·페이스 컴포넌트 이름 프로퍼티는 접근자 없는 protected라, 파생으로 열어 컴파일 타임에 안전하게 설정한다.
 * 리플렉션 이름 문자열 기입은 엔진이 프로퍼티를 개명하면 조용히 무시되므로 쓰지 않는다.
 */
UCLASS()
class WXGAME_API UWxMetaHumanComponent : public UMetaHumanComponentUE
{
	GENERATED_BODY()

public:
	/** 바디·페이스 조회에 쓸 실제 컴포넌트 이름을 지정한다. 컴포넌트 등록 전에 호출해야 한다. */
	void SetTargetComponentNames(const FString& InBodyComponentName, const FString& InFaceComponentName);
};

/** 그룸 한 종을 구성하는 에셋 쌍. 그룸을 비워두면 해당 슬롯은 생성하지 않는다. */
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
 * 메타휴먼 어셈블 산출물(페이스·그룸·복장)을 오너의 바디 메시에 조립하는 컴포넌트.
 * 캐릭터 BP마다 어셈블 BP 구성을 수작업으로 복제하는 대신, 에셋만 지정하면 등록 시점에 부착물 일체를 생성·배선한다.
 * LODSync·MetaHuman 컴포넌트가 이름 문자열로 대상을 찾는 구조라, 실제 생성된 컴포넌트 이름을 그대로 채워 이름 어긋남을 차단한다.
 * 오너가 ACharacter면 GetMesh(), 아니면(NPC 등 AActor 계열) 첫 SkeletalMeshComponent를 바디로 삼는다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxMetaHumanVisualComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	//~ Begin UActorComponent
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End UActorComponent
	
	/** 페이스 스켈레탈 메시. 비워두면 페이스와 그룸을 만들지 않는다. */
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
	FWxGroomSlot Peachfuzz;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Mustache;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	FWxGroomSlot Beard;

	/** 복장 스켈레탈 메시. 포스트프로세스 ABP가 없는 메시는 바디 리더포즈로 따라가게 배선한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Visual")
	TObjectPtr<USkeletalMesh> OutfitMesh;

private:
	/** 오너에서 부착 기준이 될 바디 메시를 찾는다. */
	USkeletalMeshComponent* ResolveBodyMesh() const;

	/** 슬롯이 채워져 있으면 페이스 자식으로 그룸 컴포넌트를 생성한다. */
	void CreateGroom(const FWxGroomSlot& Slot, const TCHAR* BaseName, USkeletalMeshComponent* AttachTarget);

	// 등록 시점에 생성한 부착물들. 재등록 가드이자 OnUnregister 정리 대상이다.
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> FaceComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGroomComponent>> GroomComponents;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> OutfitComponent;

	UPROPERTY(Transient)
	TObjectPtr<ULODSyncComponent> LODSyncComponent;

	UPROPERTY(Transient)
	TObjectPtr<UWxMetaHumanComponent> MetaHumanComponent;
};
