// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "WxItemInstance.generated.h"

class UWxItemDefinition;
class UWxItemFragment;

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

	/** 현재 충전 횟수. Charges Fragment 가 없는 아이템은 항상 0(미사용). */
	int32 GetCurrentCharges() const;

	/** Charges Fragment 의 MaxCharges. Fragment 가 없으면 0. */
	int32 GetMaxCharges() const;

	/**
	 * 권한: 충전 횟수를 설정한다. [0, MaxCharges] 로 클램프된다.
	 * 변경 브로드캐스트(OnInventoryChargeChanged)는 호출자(인벤토리 매니저) 책임이다.
	 */
	void SetCurrentCharges(int32 InCharges);

	/**
	 * 권한: 정의를 1회 바인딩한다. 인스턴스 생성 직후 FWxInventoryList::AddEntry 가 호출한다.
	 * 재바인딩은 금지되며 check(ItemDef == nullptr) 로 가드된다.
	 */
	void SetItemDef(const UWxItemDefinition* InItemDef);

protected:
	UFUNCTION()
	void OnRep_CurrentCharges(int32 OldCharges);

private:
	UPROPERTY(Replicated)
	TObjectPtr<const UWxItemDefinition> ItemDef;

	/** 인스턴스 단위 충전 횟수(에스트병 방식). Charges Fragment 가 부착된 아이템에서만 의미를 가진다. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCharges)
	int32 CurrentCharges = 0;
};
