// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"

#include "WxItemDefinition.generated.h"

class UTexture2D;
class UWxItemFragment;

UENUM(BlueprintType)
enum class EWxItemCategory : uint8
{
	None,
	Equipment,
	Consumable,
	Currency
};

/**
 * 동일한 Definition 을 참조하는 모든 인스턴스는 같은 정의를 공유하며, 런타임 가변 상태는 별도 UWxItemInstance 에서 관리한다.
 *
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWxItemCategory Category;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Item")
	TArray<TObjectPtr<UWxItemFragment>> Fragments;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	EWxItemCategory GetItemCategory() const;

	const UWxItemFragment* FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const;

	// 헤더 정의는 코딩 규칙 6 의 예외다 — 템플릿이라 cpp 로 내릴 수 없다.
	template <typename T>
	const T* FindFragmentByClass() const
	{
		static_assert(TIsDerivedFrom<T, UWxItemFragment>::IsDerived, "T must derive from UWxItemFragment");
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}
};
