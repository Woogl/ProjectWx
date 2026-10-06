// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "Types/MVVMViewModelContext.h"

#include "WxViewModel_Inventory.generated.h"

class UWxInventoryComponent;
class UWxItemDefinition;
class UWxItemInstance;
class UWxViewModel_Item;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnItemAcquired);

/**
 * 플레이어 인벤토리의 아이템 VM 을 소유하고 탭으로 거르는 Composite 뷰모델.
 * 로컬 PC 가 만들어 Global Collection 에 VM_Inventory 로 등록하고, 자기 인벤토리로 초기화한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Inventory : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UWxViewModel_Inventory();

	/** WBP 의 Global Viewmodel Collection 소스가 쓰는 이름과 같아야 한다. */
	static FMVVMViewModelContext GetPlayerContext();

	/** 로컬 PC 가 등록한 플레이어 Inventory VM. 등록 전이거나 원격 PC 뿐인 월드면 nullptr. */
	static UWxViewModel_Inventory* FindPlayer(const UObject* WorldContextObject);

	/** 시작 아이템이 획득 알림으로 뜨지 않도록 지급이 끝난 뒤에 부른다. */
	void Initialize(UWxInventoryComponent* InInventory);

	void Deinitialize();

	/** 정의 단위 합계 VM. 인벤토리에 없어도 정의의 정적 정보로 만들고, 위젯이 붙들고 있으므로 인벤토리가 비어도 유지한다. */
	UWxViewModel_Item* GetOrCreateItemViewModel(const UWxItemDefinition* ItemDef);

	/** PC 가 사는 동안 같은 인스턴스이므로 인벤토리 화면을 다시 열어도 마지막 선택이 유지된다. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCurrentCategory, Category = "Wx|Inventory", meta = (Categories = "Item.Category"))
	FGameplayTag CurrentCategory;

	/** SlotItems 중 CurrentCategory 에 속한 것. 둘 중 하나가 바뀌면 다시 거른다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> CategorizedItems;

	/**
	 * 획득(Delta>0)마다 새 VM 으로 교체되므로 토스트 위젯 간 표시 데이터가 서로 영향을 주지 않고, 획득 시점의 값으로 채운 뒤 더 갱신하지 않는다.
	 * 상태 바인딩으로 받으면 뷰 초기화 때 지난 획득이 다시 실행되므로, OnItemAcquired 이벤트의 목적지 인자로만 읽는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Inventory")
	TObjectPtr<UWxViewModel_Item> LastAcquiredItem;

	/** 뷰는 MVVM 이벤트 바인딩으로 받는다 — 엔진 이벤트 바인딩은 델리게이트 인자를 넘기지 못해 획득 VM 은 LastAcquiredItem 으로 읽는다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx|Inventory")
	FWxOnItemAcquired OnItemAcquired;

	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	void SetCurrentCategory(FGameplayTag NewCategory);

private:
	void HandleInventoryStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	/** 충전 변경은 목록 구성을 바꾸지 않으므로 만들어 둔 VM 의 값만 다시 채운다. */
	void HandleInventoryChargeChanged(UWxItemInstance* Instance, int32 NewCharges, int32 Delta);

	void RefreshItems();
	void RefreshItemViewModels();
	UWxViewModel_Item* CreateItemViewModel(const UWxItemDefinition& Definition, const UWxItemInstance* Instance);
	void RefreshCategorizedItems();

	TWeakObjectPtr<UWxInventoryComponent> Inventory;

	/** 슬롯 하나당 하나. 동일 정의가 여러 슬롯으로 나뉘어 있어도 각자 자기 슬롯을 표시한다. */
	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Item>> SlotItems;

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Item>> DefinitionItems;
};
