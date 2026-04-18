// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVM/WxViewModel.h"

#include "WxViewModel_Inventory.generated.h"

class UWxInventoryManagerComponent;
class UWxItemDefinition;

/**
 * 플레이어 인벤토리의 전역 집계/알림 ViewModel.
 *
 * 단일 싱글톤 Shell 로 GlobalCollection 에 등록되며, PlayerState 가 도착한 시점에 Initialize(InventoryManager) 로 데이터 소스를 연결한다. 역할은
 * 두 가지로 한정한다:
 *   1) 재화 Tag 기준 총 보유량 집계 (GetCurrencyAmount)
 *   2) 가장 최근 스택 변경 알림 (LastChangedCurrency/Amount/Delta) — 획득 Toast, 팝업 이펙트 등 "방금 무엇이 얼마나 변했는지" 채널
 *
 * 특정 ItemDef 의 수량/아이콘/이름 등 슬롯 단위 표시 데이터는 본 VM을 쓰지 말고 UWxViewModel_Item 을 위젯 인스턴스별로 생성해 사용한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Inventory : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UWxInventoryManagerComponent* InInventory);
	virtual void Deinitialize() override;

	/**
	 * CurrencyTag 기준 총 보유량. UMG 는 LastChangedCurrency/LastChangedAmount를 바인딩 Source 로 두고 ConversionFunction 에서 본 getter 를 호출해 표시한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx|Inventory")
	int32 GetCurrencyAmount(FGameplayTag CurrencyTag) const;

	/** 가장 최근에 변경된 재화 태그. 유효하지 않으면 비-재화 아이템 변경. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	FGameplayTag LastChangedCurrency;

	/** 가장 최근 변경 후의 총 보유량. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastChangedAmount = 0;

	/** 가장 최근의 변화량(양수: 획득, 음수: 소모). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastChangedDelta = 0;

protected:
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	TWeakObjectPtr<UWxInventoryManagerComponent> CachedInventory;

	FDelegateHandle StackChangedHandle;
};
