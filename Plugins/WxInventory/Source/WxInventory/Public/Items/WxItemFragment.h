// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "WxItemFragment.generated.h"

class UGameplayEffect;
class USkeletalMesh;
class UWxItemInstance;

/**
 * 아이템 Fragment 의 베이스 UObject.
 *
 * UWxItemDefinition 의 Fragments 컬렉션에 EditInline 인스턴스로 부착되어, 아이템에 데이터·행동을 컴포지션한다.
 * Fragment 의 인스턴스는 정의(CDO) 안에 살아있는 단일 객체이며, 같은 정의를 참조하는 모든 인스턴스가 이를 공유한다.
 *
 * 카테고리 분류는 본 Fragment 가 아니라 UWxItemDefinition::Category(EWxItemCategory) 가 직접 표현한다.
 * Fragment 는 "이 아이템이 무엇을 할 수 있는가"(기능 축)만 책임지며, 카테고리에 종속되지 않는다.
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class WXINVENTORY_API UWxItemFragment : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 권한: AddItemDefinition 으로 새 인스턴스가 생성된 직후 호출.
	 * Fragment 가 인스턴스 초기 상태를 주입할 때 사용한다. 기본 구현은 비어있음.
	 */
	virtual void OnInstanceCreated(UWxItemInstance* Instance) const;
};

/**
 * 장착 시 무기 액터의 스켈레탈 메시를 교체하는 Fragment.
 *
 * 무기 액터 자체는 캐릭터가 항상 소유하므로(WxEquipmentComponent::WeaponActor),
 * 장비 변경은 액터 스폰/디스트로이가 아닌 메시 스왑과 부착 소켓 변경으로만 수행한다.
 */
UCLASS(DisplayName = "Equippable")
class WXINVENTORY_API UWxItemFragment_Equippable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 무기 메시 컴포넌트에 적용할 스켈레탈 메시 */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** 장착 시 소유자 메시에서 이 장비를 부착할 소켓. 비어 있으면 메시 루트에 부착. */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	FName AttachSocket = TEXT("hand_r");

	/** 장착 시 적용되고 해제 시 제거되는 GE 들. 스탯 모디파이어/패시브 효과용. */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	TArray<TSubclassOf<UGameplayEffect>> EquipEffects;
};

/**
 * 사용 시 사용자에게 GameplayEffect 를 적용하는 Fragment.
 *
 * 인벤토리 매니저의 UseItemByDef 가 Effect 적용과 스택 1 차감을 함께 수행한다.
 */
UCLASS(DisplayName = "Usable")
class WXINVENTORY_API UWxItemFragment_Usable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 사용 시 사용자(소유 폰)의 ASC에 적용할 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, Category = "Usable")
	TSubclassOf<UGameplayEffect> Effect;
};

/**
 * 스택 가능 아이템임을 선언하는 Fragment.
 *
 * 본 Fragment 가 부착된 아이템은 인벤토리에서 한 슬롯에 MaxStack 개까지 머지된다.
 * 부재 시에는 한 슬롯당 1개로 강제되어 스택되지 않는다 (장비처럼 인스턴스 고유 상태가 있는 아이템에 적합).
 */
UCLASS(DisplayName = "Stackable")
class WXINVENTORY_API UWxItemFragment_Stackable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * 한 슬롯에 누적 가능한 최대 개수.
	 * 상한은 10억(1e9)으로 제한한다 — int32 합산 오버플로우 방지 및 HUD 표기 관행.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stackable", meta = (ClampMin = "1", ClampMax = "1000000000"))
	int32 MaxStack = 99;
};
