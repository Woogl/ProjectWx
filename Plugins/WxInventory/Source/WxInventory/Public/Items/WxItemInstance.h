// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Items/WxItemDefinition.h"

#include "WxItemInstance.generated.h"

/**
 * 아이템 한 자루의 런타임 인스턴스.
 *
 * UWxItemDefinition 이 정적 정의라면, 본 인스턴스는 개별 아이템의 가변 상태와
 * 수명을 담는 단위다. 인벤토리/장비 매니저가 생성·소멸을 관리하며, GameplayAbility
 * 의 SourceObject 로 활용되어 강화 수치·내구도·탄약 등 인스턴스별 데이터에
 * 접근하는 진입점이 된다.
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

	/** 정의를 1회 바인딩한다. 인스턴스 라이프사이클 시작 시점에만 호출. */
	void SetItemDef(const UWxItemDefinition* InItemDef);

	const UWxItemDefinition* GetItemDef() const;

	/** ItemDef 의 Fragment 를 위임 조회. 없으면 nullptr. */
	template <typename T>
	const T* FindFragment() const;

private:
	UPROPERTY(Replicated)
	TObjectPtr<const UWxItemDefinition> ItemDef;
};

template <typename T>
const T* UWxItemInstance::FindFragment() const
{
	return ItemDef ? ItemDef->FindFragment<T>() : nullptr;
}
