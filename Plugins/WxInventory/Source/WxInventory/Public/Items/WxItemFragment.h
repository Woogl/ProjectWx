// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

#include "WxItemFragment.generated.h"

class AActor;

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
 * 장착 시 소유자에 스폰/부착할 액터를 지정하는 Fragment.
 *
 * 액터 클래스 자체가 메시·소켓·콜리전·GAS 부여 로직 등 장비 행동을 캡슐화한다.
 * 매니저는 장착 시점에 본 클래스를 스폰하여 소유 캐릭터에 attach 하고, 해제 시점에 destroy 한다.
 */
USTRUCT(BlueprintType, DisplayName = "Equipment")
struct WXINVENTORY_API FWxItemFragment_Equipment : public FWxItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Equipment", meta = (AllowedClasses = "/Script/WxCombat.WxWeaponBase"))
	TSubclassOf<AActor> EquipmentActorClass;

	/** 장착 시 소유자 메시에서 이 장비를 부착할 소켓. 비어 있으면 메시 루트에 부착. */
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FName AttachSocket = TEXT("hand_r");
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
