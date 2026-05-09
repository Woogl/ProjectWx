// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel.h"

#include "WxViewModel_Inventory.generated.h"

class UWxInventoryManagerComponent;
class UWxItemInstance;
class UWxViewModel_Item;

/**
 * 아이템 획득 이벤트 델리게이트.
 * Delta 는 양수만 브로드캐스트되며 (소비/감소는 알리지 않음), HUD 토스트 spawn 등 일회성 표시 트리거용이다.
 * ItemVM 은 VM_Inventory 가 Def 단위로 캐시한 Def 모드 UWxViewModel_Item 으로, 같은 ItemDef 재획득 시 동일 인스턴스가 재사용된다. 수신측은 ListView ItemSource 에 그대로 넣어 Icon/DisplayName/Grade/ItemCount 를 MVVM 바인딩으로 표시할 수 있다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWxOnItemAcquired, UWxViewModel_Item*, ItemVM, int32, Delta);

/**
 * 플레이어 인벤토리의 전역 집계/알림 ViewModel.
 *
 * 단일 싱글톤 Shell 로 GlobalCollection 에 등록되며, PC 가 도착한 시점(ReceivedPlayer)에 Initialize(InventoryManager) 로 데이터 소스를 연결한다. 역할은
 * 다섯 가지로 한정한다:
 *   1) ItemDef 기준 총 보유량 집계 (GetCurrencyAmount)
 *   2) 가장 최근 스택 변경 알림 (LastChangedItemDef/Amount/Delta) — 단발성 Toast/팝업 이펙트 등 "방금 무엇이 얼마나 변했는지" 단일 채널
 *   3) 보유 중인 아이템 인스턴스 전체 목록 (AllItems) — 인벤토리 ListView 등 "전체 슬롯을 나열"하는 화면의 ItemSource 로 사용
 *   4) 카테고리 탭 표시용 필터링된 목록 (FilteredItems) — CurrentCategory 변경 또는 AllItems 갱신 시 자동 재계산
 *   5) 아이템 획득 이벤트 (OnItemAcquired) — Delta>0 마다 BP-assignable 델리게이트 Broadcast. HUD 토스트 등 즉시성 이벤트 수신용. Def 단위로 캐시된 UWxViewModel_Item 을 함께 전달하므로 수신측이 ListView/MVVM 으로 표시 데이터를 그대로 바인딩한다.
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
	 * ItemDef 기준 총 보유량. UMG 는 LastChangedItemDef/LastChangedAmount를 바인딩 Source 로 두고 ConversionFunction 에서 본 getter 를 호출해 표시한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx|Inventory")
	int32 GetCurrencyAmount(const UWxItemDefinition* ItemDef) const;

	/** 가장 최근에 변경된 ItemDef. nullptr 이면 변경 이력 없음. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<const UWxItemDefinition> LastChangedItemDef;

	/** 가장 최근 변경 후의 총 보유량. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastChangedAmount = 0;

	/** 가장 최근의 변화량(양수: 획득, 음수: 소모). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastChangedDelta = 0;

	/**
	 * 슬롯별 UWxViewModel_Item 배열. WBP_ItemSlot 이 ListView 엔트리로 이 VM 을 직접 받아 Manual 바인딩하는 것을 전제로 한다.
	 * 동일 ItemDef 가 복수 슬롯으로 분할되어 있어도 각 VM 이 자기 슬롯 인스턴스에 바인딩되어 독립적으로 수량을 표시한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> AllItems;

	/**
	 * 현재 화면이 표시 중인 카테고리. 탭 위젯이 BlueprintSetter 를 통해 갱신한다.
	 * Setter 가 CategorizedItems 재계산을 함께 트리거하므로, 직접 멤버를 쓰지 말고 SetCurrentCategory 로만 변경한다.
	 */
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCurrentCategory, Category = "Wx|Inventory")
	EWxItemCategory CurrentCategory = EWxItemCategory::Equipment;

	/**
	 * CurrentCategory 기준으로 AllItems 를 필터링한 결과. TileView/ListView 의 ItemSource 는 AllItems 가 아닌 본 프로퍼티에 바인딩한다.
	 * AllItems 변경 또는 CurrentCategory 변경 시 자동 갱신된다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> CategorizedItems;

	/**
	 * 아이템 획득 이벤트. HandleStackChanged 에서 Delta>0 일 때 Broadcast 한다.
	 * HUD 위젯이 BindEvent 로 콜백을 잡아 토스트 위젯 spawn / 자체 timer / 페이드 등 표시 정책을 자유롭게 결정한다.
	 * 전달되는 ItemVM 은 AcquisitionVMs 에 Def 단위로 캐시된 Def 모드 UWxViewModel_Item 이며, 동일 ItemDef 재획득 시 같은 인스턴스가 재사용된다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Wx|Inventory")
	FWxOnItemAcquired OnItemAcquired;

	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	void SetCurrentCategory(EWxItemCategory NewCategory);

protected:
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	void RefreshAllItems();

	void RefreshCategorizedItems();

	TWeakObjectPtr<UWxInventoryManagerComponent> CachedInventory;

	FDelegateHandle StackChangedHandle;

	/**
	 * OnItemAcquired Broadcast 용 Def-keyed VM_Item 캐시.
	 * 첫 Delta>0 발생 시 Def 모드로 NewObject 하고, 이후 같은 ItemDef 재획득 시 동일 인스턴스를 재사용한다. ItemCount 는 인벤 총량으로 자동 갱신되며 Deinitialize 에서 일괄 정리한다.
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<const UWxItemDefinition>, TObjectPtr<UWxViewModel_Item>> AcquisitionVMs;
};
