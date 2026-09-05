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
 * UWxViewModelResolver_Inventory 가 위젯별로 생성하며, 인벤토리 연결은 본 VM 이 스스로 관찰해 처리한다.
 * 인벤토리는 Experience 주입(서버) 또는 복제(클라)로 붙어 위젯보다 늦게 도착할 수 있고, 리졸버가 돌려준 인스턴스는 뷰가 교체할 수 없다.
 * 동일한 뷰모델을 유지하며 인벤토리의 등장과 제거에 맞춰 내부 연결을 교체한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Inventory : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 인벤토리가 이미 붙어 있으면 즉시 연결하고, 아니면 도착 신호를 기다린다. */
	void StartObserving(APlayerController* PC);

	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	bool bIsInventoryAvailable = false;

	/** ItemDef 기준 총 보유량. */
	UFUNCTION(BlueprintPure, Category = "Wx|Inventory")
	int32 GetCurrencyAmount(const UWxItemDefinition* ItemDef) const;

	/** nullptr 이면 변경 이력 없음. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<const UWxItemDefinition> LastChangedItemDef;

	/** 가장 최근 변경 후의 총 보유량. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastChangedAmount = 0;

	/** 가장 최근의 변화량(양수: 획득, 음수: 소모). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastChangedDelta = 0;

	/**
	 * 동일 ItemDef 가 복수 슬롯으로 분할되어 있어도 각 VM 이 자기 슬롯 인스턴스에 바인딩되어 독립적으로 수량을 표시한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> AllItems;

	/**
	 * Setter 가 CategorizedItems 재계산을 함께 트리거하므로, 직접 멤버를 쓰지 말고 SetCurrentCategory 로만 변경한다.
	 */
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCurrentCategory, Category = "Wx|Inventory")
	EWxItemCategory CurrentCategory = EWxItemCategory::Equipment;

	/**
	 * AllItems 변경 또는 CurrentCategory 변경 시 자동 갱신된다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> CategorizedItems;

	/**
	 * HandleStackChanged 에서 Delta>0 일 때 교체된다.
	 * 매번 새로 생성된 Def 모드 UWxViewModel_Item(AcquiredCount=Delta) 이므로 같은 ItemDef 를 연속 획득해도 FieldNotify 가 항상 발생하고, 토스트 위젯 간 표시 데이터가 서로 영향을 주지 않는다.
	 * 뷰 초기화 시점의 첫 실행에서는 nullptr 가 전달되므로 수신측이 유효성을 검사해야 한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<UWxViewModel_Item> LastAcquiredItem;

	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	void SetCurrentCategory(EWxItemCategory NewCategory);

protected:
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	void RefreshAllItems();

	void RefreshCategorizedItems();

	TWeakObjectPtr<UWxInventoryComponent> CachedInventory;

	FDelegateHandle StackChangedHandle;

private:
	void HandleInventoryReady(UWxInventoryComponent* Inventory);
	void HandleInventoryEnded(UWxInventoryComponent* Inventory);
	void BindSource(UWxInventoryComponent* Inventory);
	void UnbindSource();
	void HandleContentsChanged();

	void StopObserving();

	TWeakObjectPtr<APlayerController> ObservedController;

	FDelegateHandle ReadyHandle;
	FDelegateHandle EndedHandle;
	FDelegateHandle ContentsChangedHandle;
};

/**
 * 인벤토리 탐색/연결은 뷰모델이 스스로 수행하므로, 인벤토리가 아직 없어도 VM 은 만들어지고 도착 시점에 채워진다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Inventory : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
