// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "Items/WxGameplayTagStack.h"

#include "WxItemInstance.generated.h"

class UWxItemDefinition;
class UWxItemFragment;
struct FWxInventoryList;

/**
 * 아이템 한 자루의 런타임 인스턴스.
 *
 * UWxItemDefinition 이 정적 정의(데이터 자산)라면, 본 인스턴스는 개별 아이템의 가변 상태와
 * 수명을 담는 단위다. 인벤토리 매니저가 생성·소멸을 관리하며, 가변 상태(스택 수, 탄약, 강화수치,
 * 내구도 등)는 모두 StatTags 한 군데에서 GameplayTag → int32 매핑으로 표현해 새 상태 추가 시
 * 클래스 수정 없이 태그만 늘리면 된다.
 *
 * GameplayAbility 의 SourceObject 로 활용되어 인스턴스별 데이터에 접근하는 진입점이 된다.
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

	/** 권한: 태그 스택 추가. */
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	/** 권한: 태그 스택 차감. */
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	bool HasStatTag(FGameplayTag Tag) const;

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
	FWxGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	TObjectPtr<const UWxItemDefinition> ItemDef;
};
