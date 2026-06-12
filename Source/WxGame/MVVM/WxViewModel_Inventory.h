// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Inventory.generated.h"

class UWxInventoryManagerComponent;
class UWxItemInstance;
class UWxViewModel_Item;
class UUserWidget;
class UMVVMView;

/**
 * 플레이어 인벤토리의 집계/알림 ViewModel.
 *
 * UWxViewModelResolver_Inventory 가 위젯별로 생성하며, 생성 시점에 소유 PC 의 InventoryManager 를 Initialize 로 연결한다. 역할은
 * 다섯 가지로 한정한다:
 *   1) ItemDef 기준 총 보유량 집계 (GetCurrencyAmount)
 *   2) 가장 최근 스택 변경 알림 (LastChangedItemDef/Amount/Delta) — 단발성 Toast/팝업 이펙트 등 "방금 무엇이 얼마나 변했는지" 단일 채널
 *   3) 보유 중인 아이템 인스턴스 전체 목록 (AllItems) — 인벤토리 ListView 등 "전체 슬롯을 나열"하는 화면의 ItemSource 로 사용
 *   4) 카테고리 탭 표시용 필터링된 목록 (FilteredItems) — CurrentCategory 변경 또는 AllItems 갱신 시 자동 재계산
 *   5) 가장 최근 획득 알림 (LastAcquiredItem) — Delta>0 마다 새로 생성된 Def 모드 UWxViewModel_Item(AcquiredCount=Delta) 으로 교체. HUD 토스트 등 즉시성 표시는 본 프로퍼티를 View Binding 으로 수신해 ListView ItemSource 에 그대로 넣는다.
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
	 * 가장 최근 획득 알림용 ViewModel. HandleStackChanged 에서 Delta>0 일 때 교체된다.
	 * 매번 새로 생성된 Def 모드 UWxViewModel_Item(AcquiredCount=Delta) 이므로 같은 ItemDef 를 연속 획득해도 FieldNotify 가 항상 발생하고, 토스트 위젯 간 표시 데이터가 서로 영향을 주지 않는다.
	 * View 는 본 프로퍼티를 토스트 추가 함수에 OneWay 바인딩한다. 뷰 초기화 시점의 첫 실행에서는 nullptr 가 전달되므로 수신측이 유효성을 검사해야 한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<UWxViewModel_Item> LastAcquiredItem;

	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	void SetCurrentCategory(EWxItemCategory NewCategory);

protected:
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	void RefreshAllItems();

	void RefreshCategorizedItems();

	TWeakObjectPtr<UWxInventoryManagerComponent> CachedInventory;

	FDelegateHandle StackChangedHandle;
};

/**
 * VM_Inventory 용 View Bindings Resolver.
 *
 * 위젯을 소유한 AWxPlayerController 의 InventoryManager 를 끌어와 위젯별 UWxViewModel_Inventory 를 생성/초기화한다.
 * InventoryManager 는 PC 의 생성자 서브오브젝트라 위젯이 존재하는 시점엔 항상 사용 가능하다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Inventory : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
