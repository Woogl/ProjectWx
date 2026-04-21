// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

#include "WxItemFragment.generated.h"

class UGameplayEffect;
class USkeletalMesh;

/**
 * 아이템 Fragment의 베이스 USTRUCT.
 *
 * UWxItemDefinition.Fragments 에 FInstancedStruct 형태로 부착되어, 아이템에 데이터 단위 행동/속성을 컴포지션한다.
 * 가상 함수는 사용하지 않으며, 처리 로직은 외부 시스템(예: 인벤토리/장비 매니저)이 타입별로 디스패치한다.
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxItemFragment
{
	GENERATED_BODY()
};

/**
 * 장착 시 소유자가 항상 들고 있는 무기 액터의 스켈레탈 메시를 교체하는 Fragment.
 *
 * 무기 액터 자체는 캐릭터가 항상 소유하므로(WxCharacterBase::DefaultWeapon),
 * 장비 변경은 액터 스폰/디스트로이가 아닌 메시 스왑과 부착 소켓 변경으로만 수행한다.
 */
USTRUCT(BlueprintType, DisplayName = "Equipment")
struct WXINVENTORY_API FWxItemFragment_Equipment : public FWxItemFragment
{
	GENERATED_BODY()

	/** 무기 메시 컴포넌트에 적용할 스켈레탈 메시 */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** 장착 시 소유자 메시에서 이 장비를 부착할 소켓. 비어 있으면 메시 루트에 부착. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FName AttachSocket = TEXT("hand_r");
};

/**
 * 소비 시 사용자에게 GameplayEffect를 적용하는 Fragment.
 *
 * 인벤토리 매니저의 UseItemByDef 가 Effect 적용과 스택 1 차감을 함께 수행한다.
 */
USTRUCT(BlueprintType, DisplayName = "Consumable")
struct WXINVENTORY_API FWxItemFragment_Consumable : public FWxItemFragment
{
	GENERATED_BODY()

	/** 사용 시 사용자(소유 폰)의 ASC에 적용할 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, Category = "Consumable")
	TSubclassOf<UGameplayEffect> Effect;
};

/**
 * 이 아이템을 재화로 식별하는 Fragment.
 *
 * 인벤토리 매니저는 CurrencyTag 를 키로 월렛 조회/차감을 가속한다. 재화 Definition 은 일반적으로 MaxCounts 를 매우 크게 잡아 단일 슬롯에 누적된다.
 */
USTRUCT(BlueprintType, DisplayName = "Currency")
struct WXINVENTORY_API FWxItemFragment_Currency : public FWxItemFragment
{
	GENERATED_BODY()

	/** 월렛/HUD 조회 키. 예: Item.Currency.Gold */
	UPROPERTY(EditDefaultsOnly, Category = "Currency", meta = (Categories = "Item.Currency"))
	FGameplayTag CurrencyTag;
};
