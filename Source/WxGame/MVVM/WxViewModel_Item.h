// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Item.generated.h"

class UWxInventoryManagerComponent;
class UTexture2D;
class UUserWidget;
class UMVVMView;

/**
 * 단일 ItemDef 슬롯의 모든 표시 데이터를 노출하는 ViewModel.
 *
 * 위젯은 Creation Type "Create Instance"로 인스턴스를 소유하고, 자신이 표시할 ItemDef 와 PlayerState 의 InventoryManager 를 인자로 Initialize(Inventory, ItemDef) 를 호출한다. UMG 는 ItemCount/Icon/DisplayName/Grade 에 직접 바인딩을 걸고, LastDelta 로 획득/소모 애니메이션을 트리거한다.
 *
 * 정적 표시 데이터(Icon/DisplayName/Grade)는 Initialize 시점 1회 세팅되며 이후 변하지 않는다. 동적인 수량 변화만 델리게이트로 갱신된다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Item : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UWxInventoryManagerComponent* InInventory, const UWxItemDefinition* InItemDef);
	virtual void Deinitialize() override;

	/** 대상 ItemDef 의 현재 총 보유량. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 ItemCount = 0;

	/** 가장 최근 변화량(양수: 획득, 음수: 소모). 애니메이션 트리거용. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	int32 LastDelta = 0;

	/** 슬롯 아이콘. Initialize 시점에 Soft 참조를 동기 로드해 세팅. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<UTexture2D> Icon;

	/** 슬롯 표시 이름. 로컬라이즈 대상. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	FText DisplayName;

	/** 슬롯 아이템 등급. 색상/이펙트 분기 키. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	EWxItemGrade Grade = EWxItemGrade::Common;

protected:
	void HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta);

	TWeakObjectPtr<UWxInventoryManagerComponent> CachedInventory;

	TWeakObjectPtr<const UWxItemDefinition> TargetItemDef;

	FDelegateHandle StackChangedHandle;
};

/**
 * UWxViewModel_Item 전용 View Bindings Resolver.
 *
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 선택하면 인스펙터에서 ItemToDisplay 를 직접 지정할 수 있다. 이후 WBP 는 Event Graph/베이스 클래스 없이도 슬롯이 자동 구성된다.
 *
 * 동작:
 *   1) UserWidget->GetOwningPlayer()->PlayerState 로 AWxPlayerState 해석
 *   2) GetInventoryManager() 로 UWxInventoryManagerComponent 획득
 *   3) UWxViewModel_Item 을 NewObject 로 생성하고 Initialize 호출
 *
 * PlayerState 가 아직 복제되지 않았다면 Initialize 없이 Shell 상태로 반환된다 (InventoryManager 확보 후 외부에서 재초기화 필요).
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
