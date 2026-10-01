// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "Types/MVVMViewModelContext.h"

#include "WxViewModel_Inventory.generated.h"

class UWxViewModel_Item;

/**
 * 플레이어 인벤토리의 아이템 VM 을 소유하고 탭으로 거르는 Composite 뷰모델.
 * 인벤토리 도메인은 알지 않는다 — 로컬 PC 가 만들어 Global Collection 에 VM_Inventory 로 등록하고, 아이템 VM 을 채워 넣는다.
 *
 * 아이템 VM 의 SourceObject 는 슬롯 VM 이면 아이템 인스턴스, 합계 VM 이면 아이템 정의다.
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

	/** 슬롯 하나당 하나. 동일 정의가 여러 슬롯으로 나뉘어 있어도 각자 자기 슬롯을 표시한다. 탭 필터를 다시 건다. */
	void SetItems(TArray<TObjectPtr<UWxViewModel_Item>> InItems);
	const TArray<TObjectPtr<UWxViewModel_Item>>& GetItems() const;

	/** 정의 단위 합계 VM. 위젯이 붙들고 있으므로 인벤토리가 비어도 유지한다. */
	UWxViewModel_Item* FindItemViewModel(const UObject* Source) const;
	void AddItemViewModel(UWxViewModel_Item* ItemViewModel);
	const TArray<TObjectPtr<UWxViewModel_Item>>& GetItemViewModels() const;

	void SetLastAcquiredItem(UWxViewModel_Item* InLastAcquiredItem);

	/** PC 가 사는 동안 같은 인스턴스이므로 인벤토리 화면을 다시 열어도 마지막 선택이 유지된다. */
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCurrentCategory, Category = "Wx|Inventory", meta = (Categories = "Item.Category"))
	FGameplayTag CurrentCategory;

	/** Items 중 CurrentCategory 에 속한 것. 둘 중 하나가 바뀌면 다시 거른다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TArray<TObjectPtr<UWxViewModel_Item>> CategorizedItems;

	/**
	 * 획득(Delta>0)마다 새 VM 으로 교체되므로 같은 정의를 연속 획득해도 FieldNotify 가 항상 발생하고, 토스트 위젯 간 표시 데이터가 서로 영향을 주지 않는다.
	 * 획득 시점의 값으로 채운 뒤 더 갱신하지 않는다.
	 * 뷰 초기화 시점의 첫 실행에서는 nullptr 가 전달되므로 수신측이 유효성을 검사해야 한다.
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Inventory")
	TObjectPtr<UWxViewModel_Item> LastAcquiredItem;

	UFUNCTION(BlueprintCallable, Category = "Wx|Inventory")
	void SetCurrentCategory(FGameplayTag NewCategory);

private:
	void RefreshCategorizedItems();

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Item>> Items;

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Item>> ItemViewModels;
};
