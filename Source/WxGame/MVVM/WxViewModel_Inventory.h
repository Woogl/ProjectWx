// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Inventory.generated.h"

class APlayerController;
class UWxInventoryComponent;
class UWxItemInstance;
class UWxViewModel_Item;
class UUserWidget;
class UMVVMView;

/**
 * 플레이어 인벤토리를 관찰해 아이템 VM 을 소유하고 채우는 Composite 뷰모델.
 * 아이템 VM 은 WxUI 에 있어 인벤토리를 모르므로 표시 데이터는 전부 여기서 공급한다.
 *
 * 인벤토리는 컨트롤러와 함께 생기지만 클라에선 복제로 위젯보다 늦게 도착할 수 있고, 리졸버가 돌려준 인스턴스는 뷰가 교체할 수 없다.
 * 그래서 뷰모델은 그대로 두고 인벤토리의 등장·제거를 스스로 관찰해 내부 연결만 교체한다.
 *
 * 아이템 VM 의 SourceObject 는 슬롯 VM 이면 ItemInstance, 합계 VM 이면 ItemDefinition 이다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Inventory : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * PlayerController 하나당 하나. 없으면 PC 를 Outer 로 만들어 관찰을 시작한다.
	 * PC 를 Outer 로 둬도 생존은 보장되지 않는다 — 이 VM 이나 자식 VM 이 루트에서 강한 참조로 닿는 동안만 유지된다.
	 */
	static UWxViewModel_Inventory* GetOrCreate(APlayerController* PC);

	/** 공유본을 사용하는 모든 화면이 사용을 마친 뒤에만 호출한다. */
	virtual void Deinitialize() override;

	/**
	 * ItemDef 의 총 보유량을 보여 주는 합계 VM. ItemDef 가 곧 공유 키다.
	 * 인벤토리가 아직 없어도 정적 정보로 만들어지며, 인벤토리가 사라졌다 다시 생겨도 같은 VM 을 계속 채운다.
	 */
	UWxViewModel_Item* GetOrCreateItemViewModel(const UWxItemDefinition* ItemDef);

	/** 슬롯 하나당 하나. 동일 ItemDef 가 복수 슬롯으로 분할되어 있어도 각자 자기 슬롯의 수량을 표시한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> AllItems;

	/** 공유본이 들고 있으므로 인벤토리 화면을 다시 열어도 마지막 선택이 유지된다. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCurrentCategory, Category = "Wx|Inventory")
	EWxItemCategory CurrentCategory = EWxItemCategory::Equipment;

	/** AllItems 중 CurrentCategory 에 속한 것. 둘 중 하나가 바뀌면 다시 거른다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> CategorizedItems;

	/**
	 * 획득(Delta>0)마다 새 VM 으로 교체되므로 같은 ItemDef 를 연속 획득해도 FieldNotify 가 항상 발생하고, 토스트 위젯 간 표시 데이터가 서로 영향을 주지 않는다.
	 * 획득 시점의 값으로 채운 뒤 더 갱신하지 않는다.
	 * 뷰 초기화 시점의 첫 실행에서는 nullptr 가 전달되므로 수신측이 유효성을 검사해야 한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<UWxViewModel_Item> LastAcquiredItem;

	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	void SetCurrentCategory(EWxItemCategory NewCategory);

private:
	/** PC별 일회 초기화 계약을 팩토리 내부로 제한한다. */
	void Initialize(APlayerController* PC);

	void HandleInventoryReady(UWxInventoryComponent* Inventory);
	void HandleInventoryEnded(UWxInventoryComponent* Inventory);
	void BindSource(UWxInventoryComponent* Inventory);
	void UnbindSource();

	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	/** 슬롯 수량·충전 변경은 목록 구성을 바꾸지 않으므로 만들어 둔 VM 의 값만 다시 채운다. */
	void HandleInstanceChanged(UWxItemInstance* Instance, int32 NewValue, int32 Delta);

	void RefreshAllItems();
	void RefreshCategorizedItems();
	void RefreshItemViewModels();

	/** 인벤토리가 없으면 수량 0 과 정의 아이콘으로 채운다. */
	void RefreshItemViewModel(UWxViewModel_Item& ItemViewModel) const;

	UWxViewModel_Item* CreateItemViewModel(const UObject* Source);

	TWeakObjectPtr<APlayerController> ObservedController;
	TWeakObjectPtr<UWxInventoryComponent> CachedInventory;

	/** GetOrCreateItemViewModel 로 만든 합계 VM. 위젯이 붙들고 있으므로 인벤토리가 사라져도 유지한다. */
	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Item>> ItemViewModels;

	FDelegateHandle ReadyHandle;
	FDelegateHandle EndedHandle;
	FDelegateHandle StackChangedHandle;
	FDelegateHandle SlotChangedHandle;
	FDelegateHandle ChargeChangedHandle;
	FDelegateHandle ContentsChangedHandle;
};

/** 위젯을 소유한 PlayerController 의 공유 인벤토리 뷰모델을 얻는다. */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Inventory : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};

/** 고정 아이템의 합계 VM 을 공유 인벤토리 뷰모델에서 얻는다. */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Item : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Wx|Inventory")
	TObjectPtr<UWxItemDefinition> ItemToDisplay;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
