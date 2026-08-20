// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Inventory.generated.h"

class APlayerController;
class UWxInventoryManagerComponent;
class UWxItemInstance;
class UWxViewModel_Item;
class UUserWidget;
class UMVVMView;

/**
 * 플레이어 인벤토리의 집계/알림 ViewModel.
 *
 * UWxViewModelResolver_Inventory 가 위젯별로 생성하며, 인벤토리 연결은 본 VM 이 스스로 관찰해 처리한다.
 * 인벤토리는 GameMode 주입(서버) 또는 복제(클라)로 붙어 위젯보다 늦게 도착할 수 있고, 리졸버가 돌려준 인스턴스는 뷰가 교체할 수 없다.
 * 그래서 인스턴스는 고정한 채 도착 신호를 받아 내부 상태(Initialize)만 갈아끼운다 — UWxViewModel_BossCharacter 와 같은 구조다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Inventory : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 인벤토리가 이미 붙어 있으면 즉시 연결하고, 아니면 도착 신호를 기다린다. */
	void StartObserving(APlayerController* PC);

	void Initialize(UWxInventoryManagerComponent* InInventory);
	virtual void Deinitialize() override;

	virtual void BeginDestroy() override;

	/**
	 * ItemDef 기준 총 보유량.
	 * UMG 는 LastChangedItemDef/LastChangedAmount를 바인딩 Source 로 두고 ConversionFunction 에서 본 getter 를 호출해 표시한다.
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
	 * WBP_ItemSlot 이 ListView 엔트리로 이 VM 을 직접 받아 Manual 바인딩하는 것을 전제로 한다.
	 * 동일 ItemDef 가 복수 슬롯으로 분할되어 있어도 각 VM 이 자기 슬롯 인스턴스에 바인딩되어 독립적으로 수량을 표시한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> AllItems;

	/**
	 * 탭 위젯이 BlueprintSetter 를 통해 갱신한다.
	 * Setter 가 CategorizedItems 재계산을 함께 트리거하므로, 직접 멤버를 쓰지 말고 SetCurrentCategory 로만 변경한다.
	 */
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCurrentCategory, Category = "Wx|Inventory")
	EWxItemCategory CurrentCategory = EWxItemCategory::Equipment;

	/**
	 * CurrentCategory 기준으로 AllItems 를 필터링한 결과.
	 * TileView/ListView 의 ItemSource 는 AllItems 가 아닌 본 프로퍼티에 바인딩한다.
	 * AllItems 변경 또는 CurrentCategory 변경 시 자동 갱신된다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> CategorizedItems;

	/**
	 * HandleStackChanged 에서 Delta>0 일 때 교체된다.
	 * 매번 새로 생성된 Def 모드 UWxViewModel_Item(AcquiredCount=Delta) 이므로 같은 ItemDef 를 연속 획득해도 FieldNotify 가 항상 발생하고, 토스트 위젯 간 표시 데이터가 서로 영향을 주지 않는다.
	 * View 는 본 프로퍼티를 토스트 추가 함수에 OneWay 바인딩한다.
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

	TWeakObjectPtr<UWxInventoryManagerComponent> CachedInventory;

	FDelegateHandle StackChangedHandle;

private:
	/** 관찰 중인 PC 의 것이면 연결하고 관찰을 끝낸다. */
	void HandleInventoryReady(UWxInventoryManagerComponent* Inventory);

	/** 도착 신호 구독을 해제한다. 연결 성공 시와 소멸 시 모두 여기로 모은다. */
	void StopObserving();

	TWeakObjectPtr<APlayerController> ObservedController;

	FDelegateHandle InventoryReadyHandle;
};

/**
 * 위젯별로 관찰형 인벤토리 뷰모델(UWxViewModel_Inventory)을 생성해 돌려준다.
 * 인벤토리 탐색/연결은 뷰모델이 스스로 수행하므로, 인벤토리가 아직 없어도 VM 은 만들어지고 도착 시점에 채워진다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Inventory : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
