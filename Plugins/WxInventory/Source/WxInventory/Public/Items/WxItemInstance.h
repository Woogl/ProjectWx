// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "WxItemInstance.generated.h"

class UTexture2D;
class UWxItemDefinition;
class UWxItemFragment;

/**
 * UWxItemDefinition 이 정적 정의(데이터 자산)라면, 본 인스턴스는 개별 아이템의 수명/식별 단위다.
 * 인벤토리 매니저가 생성·소멸을 관리하며, 슬롯 단위 델리게이트의 안정 식별자 역할을 한다.
 *
 * 아이템 사용 GE 컨텍스트의 SourceObject 로 실려, 효과 측이 인스턴스별 데이터(Definition 의 Fragment 등)에 접근하는 진입점이 된다.
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

	const UWxItemFragment* FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const;

	// 헤더 정의는 코딩 규칙 6 의 예외다 — 템플릿이라 cpp 로 내릴 수 없다.
	template <typename T>
	const T* FindFragmentByClass() const
	{
		static_assert(TIsDerivedFrom<T, UWxItemFragment>::IsDerived, "T must derive from UWxItemFragment");
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}

	/** Charges Fragment 가 없는 아이템은 항상 0(미사용). */
	int32 GetCurrentCharges() const;

	/** Charges Fragment 가 없으면 0. */
	int32 GetMaxCharges() const;

	/**
	 * 현재 상태 표시 아이콘(텍스처 또는 머터리얼).
	 * 충전형이면 Charges Fragment 의 ChargeIcons[CurrentCharges] 를, 없으면 Definition 의 기본 Icon 을 반환한다.
	 */
	TSoftObjectPtr<UObject> GetDisplayIcon() const;

	/**
	 * 권한: 충전 횟수는 [0, MaxCharges] 로 클램프된다.
	 * 변경 브로드캐스트(OnInventoryChargeChanged)는 호출자(인벤토리 매니저) 책임이다.
	 */
	void SetCurrentCharges(int32 InCharges);

	/**
	 * 권한: 정의를 1회 바인딩한다.
	 * 인스턴스 생성 직후 FWxInventoryList::AddEntry 가 호출한다.
	 */
	void SetItemDef(const UWxItemDefinition* InItemDef);

protected:
	UFUNCTION()
	void OnRep_CurrentCharges(int32 OldCharges);

private:
	UPROPERTY(Replicated)
	TObjectPtr<const UWxItemDefinition> ItemDef;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentCharges)
	int32 CurrentCharges = 0;
};
