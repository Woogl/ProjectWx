// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Item.generated.h"

class UWxInventoryManagerComponent;
class UWxItemInstance;
class UTexture2D;
class UUserWidget;
class UMVVMView;

/**
 * 단일 슬롯(또는 ItemDef 합계) 표시 데이터를 노출하는 ViewModel.
 *
 * 두 가지 초기화 모드를 지원한다:
 *   - Initialize(Inventory, ItemInstance) : 특정 슬롯에 바인딩. ListView 엔트리처럼 동일 ItemDef 가 분할된 슬롯을 각자 표현해야 할 때 사용. 슬롯 단위 델리게이트에 구독한다.
 *   - Initialize(Inventory, ItemDef)      : ItemDef 합계에 바인딩. HUD 재화 등 "해당 아이템 총 보유량" 을 표시할 때 사용(Resolver 경로). 합계 델리게이트에 구독한다.
 *
 * UMG 는 TotalCount/Icon/DisplayName/Grade 에 직접 바인딩을 건다. 정적 표시 데이터(Icon/DisplayName/Grade)는 Initialize 시점 1회 세팅되며 이후 변하지 않는다. Icon 은 Soft 참조 그대로 전달되므로, View 측은 UCommonLazyImage 의 SetBrushFromLazyDisplayAsset 으로 비동기 로드한다.
 *
 * AcquiredCount 는 토스트 등 1회용 표시를 위한 채널이다. 본 VM 자체는 갱신하지 않으며, 외부(VM_Inventory) 가 broadcast 직전에 Delta 를 써넣어 OneTime 바인딩으로 소비된다. FieldNotify 가 아니므로 OneWay 바인딩에는 사용하지 않는다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Item : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 슬롯 단위 바인딩. ListView 엔트리처럼 특정 인스턴스를 표현할 때 사용. */
	void Initialize(UWxInventoryManagerComponent* InInventory, UWxItemInstance* InInstance);

	/** ItemDef 합계 바인딩. HUD 재화 등 정적 경로에서 사용. */
	void Initialize(UWxInventoryManagerComponent* InInventory, const UWxItemDefinition* InItemDef);

	virtual void Deinitialize() override;

	/** 슬롯 모드에서 바인딩된 인스턴스. Def 모드이거나 미초기화 상태면 nullptr. */
	UWxItemInstance* GetTargetInstance() const;

	/** 대상 ItemDef 의 현재 총 보유량. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 TotalCount = 0;

	/**
	 * 토스트 등 1회용 표시 채널. 획득 이벤트 broadcast 직전에 Delta 가 기록되며, 수신측은 OneTime 바인딩으로 읽는다.
	 * FieldNotify 미부착 — OneWay 바인딩 대상 아님.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Inventory")
	int32 AcquiredCount = 0;

	/**
	 * 슬롯 아이콘의 Soft 참조. View 측 UCommonLazyImage 가 비동기 로드/수명 관리한다.
	 * VM 은 Definition 의 Soft 참조를 그대로 노출만 하며, LoadSynchronous 를 호출하지 않는다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 슬롯 표시 이름. 로컬라이즈 대상. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	FText DisplayName;

	/** 슬롯 아이템 등급. 색상/이펙트 분기 키. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	EWxItemGrade Grade = EWxItemGrade::Common;

protected:
	/** ItemDef 합계 모드 핸들러. */
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	/** 슬롯 모드 핸들러. */
	void HandleSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta);

	/** Icon/Name/Grade 세팅 및 초기 TotalCount 갱신 공통 루틴. */
	void ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef);

	TWeakObjectPtr<UWxInventoryManagerComponent> CachedInventory;

	TWeakObjectPtr<const UWxItemDefinition> TargetItemDef;

	TWeakObjectPtr<UWxItemInstance> TargetInstance;

	FDelegateHandle StackChangedHandle;

	FDelegateHandle SlotChangedHandle;
};

/**
 * UWxViewModel_Item 전용 View Bindings Resolver.
 *
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 선택하면 인스펙터에서 ItemToDisplay 를 직접 지정할 수 있다. 이후 WBP 는 Event Graph/베이스 클래스 없이도 슬롯이 자동 구성된다.
 *
 * 동작:
 *   1) UserWidget->GetOwningPlayer() 로 AWxPlayerController 해석
 *   2) GetInventoryManager() 로 UWxInventoryManagerComponent 획득
 *   3) UWxViewModel_Item 을 NewObject 로 생성하고 Initialize 호출
 *
 * PC 가 아직 복제되지 않았다면 Initialize 없이 Shell 상태로 반환된다 (InventoryManager 확보 후 외부에서 재초기화 필요).
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Item : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	/** 이 슬롯이 표시할 아이템 정의. WBP View Bindings 인스펙터에서 지정. */
	UPROPERTY(EditAnywhere, Category = "Wx|Inventory")
	TObjectPtr<UWxItemDefinition> ItemToDisplay;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
