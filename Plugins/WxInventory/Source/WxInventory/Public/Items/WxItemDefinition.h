// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/SubclassOf.h"

#include "Items/WxItemFragment.h"

#include "WxItemDefinition.generated.h"

class UTexture2D;

/**
 * 아이템 등급. 색상/이펙트/드롭 가중치 등의 분기 키로 사용.
 */
UENUM(BlueprintType)
enum class EWxItemGrade : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

/**
 * 아이템의 정적 정의.
 *
 * Fragment 컴포지션으로 아이템의 속성/행동을 선언한다. 동일한 Definition 을
 * 참조하는 모든 인스턴스는 같은 정의를 공유하며, 런타임 가변 상태는 별도
 * Item Instance 에서 관리한다.
 *
 * 참고: Lyra 의 ULyraInventoryItemDefinition 이 UObject CDO 기반인 것과 달리,
 * 본 클래스는 UPrimaryDataAsset 기반 에셋 인스턴스로 사용된다. 참조 시
 * TSubclassOf<> 가 아닌 TObjectPtr<UWxItemDefinition> 를 사용.
 */
UCLASS(BlueprintType)
class WXINVENTORY_API UWxItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UWxItemDefinition();

	/** UI 표시용 이름. 로컬라이즈 대상. */
	UPROPERTY(EditDefaultsOnly, Category = "Display")
	FText DisplayName;

	/** UI 표시용 설명. 로컬라이즈 대상. */
	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;

	/** UI 표시용 아이콘. 비동기 로드 권장. */
	UPROPERTY(EditDefaultsOnly, Category = "Display")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 아이템 등급. */
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	EWxItemGrade Grade;

	/** 한 슬롯에 쌓을 수 있는 최대 스택 수. 1이면 비스택 아이템. */
	UPROPERTY(EditDefaultsOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxCounts;

	/**
	 * 이 아이템에 부착된 Fragment 모음.
	 * 베이스(FWxItemFragment) 자체는 추가할 수 없도록 ExcludeBaseStruct 적용.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Item", meta = (BaseStruct = "/Script/WxInventory.WxItemFragment", ExcludeBaseStruct))
	TArray<FInstancedStruct> Fragments;

	/** PrimaryAssetId. AssetManager 등록 시 사용. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * 첫 번째로 일치하는 Fragment 포인터 반환. 없으면 nullptr.
	 * 베이스 FWxItemFragment 의 파생형만 허용한다.
	 */
	template <typename T>
	const T* FindFragment() const;
};

template <typename T>
const T* UWxItemDefinition::FindFragment() const
{
	static_assert(TIsDerivedFrom<T, FWxItemFragment>::IsDerived, "T must derive from FWxItemFragment");

	for (const FInstancedStruct& Fragment : Fragments)
	{
		if (const T* Ptr = Fragment.GetPtr<T>())
		{
			return Ptr;
		}
	}
	return nullptr;
}
