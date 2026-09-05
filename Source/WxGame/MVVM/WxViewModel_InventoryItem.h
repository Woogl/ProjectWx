// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "MVVM/WxViewModel_Item.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_InventoryItem.generated.h"

class UWxInventoryComponent;
class UWxItemInstance;
class UUserWidget;
class UMVVMView;

/**
 * 단일 슬롯(또는 ItemDef 합계) 표시 데이터를 노출하는 ViewModel.
 *
 * Initialize(Inventory, ItemInstance) 는 특정 슬롯에 바인딩해 슬롯 단위 델리게이트를 구독한다.
 * Initialize(Inventory, ItemDef) 는 ItemDef 합계에 바인딩해 합계 델리게이트를 구독한다.
 *
 * 정적 표시 데이터(DisplayName/Grade/GradeColor/MaxCharges)는 Initialize 시점 1회 세팅되며 이후 변하지 않는다.
 * 충전형의 Icon 은 충전량 변경 시 ChargeIcons[CurrentCharges] 로 함께 갱신된다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InventoryItem : public UWxViewModel_Item
{
	GENERATED_BODY()

public:
	void StartObserving(APlayerController* PC, const UWxItemDefinition* InItemDef);
	void Initialize(UWxInventoryComponent* InInventory, UWxItemInstance* InInstance);

	void Initialize(UWxInventoryComponent* InInventory, const UWxItemDefinition* InItemDef);

	virtual void Deinitialize() override;

	/** 슬롯 모드에서 바인딩된 인스턴스. Def 모드이거나 미초기화 상태면 nullptr. */
	UWxItemInstance* GetTargetInstance() const;

	/**
	 * 소비 아이템이 하나뿐이라 바인딩된 아이템과 무관하게 그 하나를 사용한다.
	 * 사용 가능 여부는 요청을 받은 어빌리티가 판정한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	bool RequestUseConsumable();

	/** 슬롯 모드는 해당 슬롯의 스택 수, Def 모드는 ItemDef 의 총 보유량. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 TotalCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	bool bIsInventoryAvailable = false;

	/**
	 * 슬롯 모드는 바인딩 인스턴스, Def 모드는 첫 인스턴스 기준.
	 * 충전형(Charges Fragment)이 아니면 0.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 CurrentCharges = 0;

	/** Charges Fragment 의 MaxCharges. 충전형이 아니면 0. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 MaxCharges = 0;

	/**
	 * 토스트 등 1회용 표시 채널.
	 * 본 VM 이 아니라 VM_Inventory 가 획득 broadcast 직전에 Delta 를 써넣고, 수신측은 OneTime 바인딩으로 읽는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Inventory")
	int32 AcquiredCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	EWxItemGrade Grade = EWxItemGrade::Common;

	/** Grade Fragment 의 Color 에서 가져온다(Fragment 부재 시 Common 기본색). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	FLinearColor GradeColor = FLinearColor::White;

protected:
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	void HandleSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta);

	/** 슬롯/Def 모드 공통. */
	void HandleChargeChanged(UWxItemInstance* Instance, int32 NewCharges, int32 Delta);

	void ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef);

	void RefreshChargeIcon();

	TWeakObjectPtr<UWxInventoryComponent> CachedInventory;

	UPROPERTY(Transient)
	TObjectPtr<const UWxItemDefinition> TargetItemDef;

	TWeakObjectPtr<UWxItemInstance> TargetInstance;

	FDelegateHandle StackChangedHandle;

	FDelegateHandle SlotChangedHandle;

	FDelegateHandle ChargeChangedHandle;

	void StopObserving();
	void BindSource(UWxInventoryComponent* Inventory);
	void UnbindSource();
	void HandleInventoryReady(UWxInventoryComponent* Inventory);
	void HandleInventoryEnded(UWxInventoryComponent* Inventory);
	void RefreshFromSource();
	void HandleContentsChanged();
	TWeakObjectPtr<APlayerController> ObservedController;
	FDelegateHandle ReadyHandle;
	FDelegateHandle EndedHandle;
	FDelegateHandle ContentsChangedHandle;
};

/**
 * 고정 아이템의 정적 정보를 먼저 제공하고 플레이어 인벤토리의 등장과 제거를 관찰한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Item : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Wx|Inventory")
	TObjectPtr<UWxItemDefinition> ItemToDisplay;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
