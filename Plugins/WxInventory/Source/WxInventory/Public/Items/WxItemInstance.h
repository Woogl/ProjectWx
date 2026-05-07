// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "WxItemInstance.generated.h"

class UWxItemDefinition;
class UWxItemFragment;
struct FWxInventoryList;

/**
 * 아이템 한 자루의 런타임 인스턴스.
 *
 * UWxItemDefinition 이 정적 정의(데이터 자산)라면, 본 인스턴스는 개별 아이템의 수명/식별 단위다.
 * 인벤토리 매니저가 생성·소멸을 관리하며, 슬롯 단위 델리게이트의 안정 식별자 역할을 한다.
 *
 * GameplayAbility 의 SourceObject 로 활용되어 인스턴스별 데이터(Definition 의 Fragment 등)에 접근하는 진입점이 된다.
 */
UCLASS(BlueprintType)
class WXINVENTORY_API UWxItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UWxItemInstance();

	//~ Begin UObject interface
	virtual bool IsSupportedForNetworking() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	const UWxItemDefinition* GetItemDef() const;

	/** Definition 의 Fragment 조회를 위임. */
	const UWxItemFragment* FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const;

	template <typename T>
	const T* FindFragmentByClass() const
	{
		static_assert(TIsDerivedFrom<T, UWxItemFragment>::IsDerived, "T must derive from UWxItemFragment");
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}

private:
	friend FWxInventoryList;

	/** 정의를 1회 바인딩한다. 인스턴스 라이프사이클 시작 시점에만 호출. */
	void SetItemDef(const UWxItemDefinition* InItemDef);

	UPROPERTY(Replicated)
	TObjectPtr<const UWxItemDefinition> ItemDef;
};
