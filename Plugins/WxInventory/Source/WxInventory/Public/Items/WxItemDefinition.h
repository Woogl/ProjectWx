// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"

#include "WxItemDefinition.generated.h"

class UTexture2D;
class UWxItemFragment;

/**
 * 아이템 등급. 색상/이펙트/드롭 가중치 등의 분기 키로 사용.
 */
UENUM(BlueprintType)
enum class EWxItemGrade : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

/**
 * 아이템의 분류. 기능과 UI 출력 형태의 1차 분기 축으로 사용된다(인벤토리 탭, 상점 카테고리, 사용/장착 가능 여부 등).
 *
 * 데이터 자산이 UWxItemDefinition::Category 필드로 직접 선언한다. Fragment 구성과 독립적이므로,
 * 같은 카테고리 안에서도 Fragment 조합으로 세부 행동을 차별화할 수 있다.
 */
UENUM(BlueprintType)
enum class EWxItemCategory : uint8
{
	None,
	Equipment,
	Consumable,
	Currency
};

/**
 * 아이템의 정적 정의.
 *
 * UPrimaryDataAsset 기반의 데이터 자산 인스턴스로 사용된다. 동일한 Definition 을 참조하는 모든
 * 인스턴스는 같은 정의를 공유하며, 런타임 가변 상태는 별도 UWxItemInstance 에서 관리한다.
 *
 * Fragment 컴포지션으로 아이템의 속성/행동을 선언한다. Fragment 자체는 UObject(EditInline)이며
 * Fragment 베이스 가상 함수(OnInstanceCreated)로 인스턴스 초기 상태를 주입한다.
 */
UCLASS(BlueprintType)
class WXINVENTORY_API UWxItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UWxItemDefinition();

	/** UI 표시용 이름. 로컬라이즈 대상. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	/** UI 표시용 설명. 로컬라이즈 대상. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;

	/** UI 표시용 아이콘. 비동기 로드 권장. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 아이템 등급. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWxItemGrade Grade;

	/** 아이템 카테고리. UI 분류와 기능 분기의 1차 축. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWxItemCategory Category;

	/**
	 * 이 아이템에 부착된 Fragment 모음.
	 * Instanced 로 선언되어 디테일에서 EditInline 으로 추가/편집된다.
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Item")
	TArray<TObjectPtr<UWxItemFragment>> Fragments;

	/** PrimaryAssetId. AssetManager 등록 시 사용. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** 카테고리 필드 값을 그대로 반환한다. UI/기능 분기에서 사용. */
	EWxItemCategory GetItemCategory() const;

	/** 첫 번째로 일치하는 Fragment 포인터 반환. 없으면 nullptr. */
	const UWxItemFragment* FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const;

	template <typename T>
	const T* FindFragmentByClass() const
	{
		static_assert(TIsDerivedFrom<T, UWxItemFragment>::IsDerived, "T must derive from UWxItemFragment");
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}
};
