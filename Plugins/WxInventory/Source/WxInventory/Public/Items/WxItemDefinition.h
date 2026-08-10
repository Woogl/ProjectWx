// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"

#include "WxItemDefinition.generated.h"

class UTexture2D;
class UWxItemFragment;

/**
 * 아이템의 분류. 기능과 UI 출력 형태의 1차 분기 축으로 사용된다(인벤토리 탭, 상점 카테고리, 사용/장착 가능 여부 등).
 *
 * Fragment 구성과 독립적이므로, 같은 카테고리 안에서도 Fragment 조합으로 세부 행동을 차별화할 수 있다.
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
 * 동일한 Definition 을 참조하는 모든 인스턴스는 같은 정의를 공유하며, 런타임 가변 상태는 별도 UWxItemInstance 에서 관리한다.
 *
 * Fragment 컴포지션으로 아이템의 속성/행동을 선언한다.
 * Fragment 베이스의 OnInstanceCreated 가 인스턴스 초기 상태를 주입한다.
 */
UCLASS(BlueprintType)
class WXINVENTORY_API UWxItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UWxItemDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;

	/** UI 표시용 아이콘. 비동기 로드 권장. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWxItemCategory Category;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Item")
	TArray<TObjectPtr<UWxItemFragment>> Fragments;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	EWxItemCategory GetItemCategory() const;

	const UWxItemFragment* FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const;

	// 헤더 정의는 코딩 규칙 6 의 예외다 — 템플릿이라 cpp 로 내릴 수 없고, 실제 조회는 위 오버로드가 한다.
	template <typename T>
	const T* FindFragmentByClass() const
	{
		static_assert(TIsDerivedFrom<T, UWxItemFragment>::IsDerived, "T must derive from UWxItemFragment");
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}
};
