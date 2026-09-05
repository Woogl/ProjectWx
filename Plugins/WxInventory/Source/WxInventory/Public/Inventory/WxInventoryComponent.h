// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Items/WxRewardTableRow.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "WxInventoryComponent.generated.h"

class UWxItemDefinition;
class UWxItemInstance;
class UWxInventoryComponent;
struct FWxInventoryList;

USTRUCT(BlueprintType)
struct FWxInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FWxInventoryEntry();

	UWxItemInstance* GetInstance() const;
	int32 GetStackCount() const;

private:
	friend FWxInventoryList;

	UPROPERTY()
	TObjectPtr<UWxItemInstance> Instance;

	UPROPERTY()
	int32 StackCount;

	/** 클라이언트 델타 계산용. */
	UPROPERTY(NotReplicated)
	int32 LastObservedCount;
};

/**
 * FWxInventoryList 의 변경 메서드가 슬롯별 변경 결과를 호출자에게 돌려주는 값 객체.
 * 함수 결과 전용이라 USTRUCT 이 아니며, 담는 포인터도 GC 추적이 필요 없는 transient 다.
 */
struct FWxInventoryChangeResult
{
	UWxItemInstance* Instance = nullptr;
	int32 NewStackCount = 0;
	int32 Delta = 0;
};

USTRUCT(BlueprintType)
struct FWxInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FWxInventoryList();

	explicit FWxInventoryList(UActorComponent* InOwnerComponent);

	//~ Begin FFastArraySerializer
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~ End FFastArraySerializer

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

	/**
	 * 권한: 새 인스턴스를 생성해 신규 엔트리로 추가.
	 * Fragment 의 OnInstanceCreated 가 호출된다.
	 */
	UWxItemInstance* AddEntry(const UWxItemDefinition* ItemDef, int32 StackCount);

	/** 권한: 인스턴스에 해당하는 엔트리를 통째로 제거. */
	void RemoveEntry(UWxItemInstance* Instance);

	/** 권한: 갱신 후 수량을 반환한다(MarkItemDirty 포함). */
	int32 AddToEntryStack(int32 EntryIndex, int32 Amount);

	/**
	 * 권한: ItemDef 를 NumToConsume 만큼 슬롯 순서대로 차감하고 0 이 된 슬롯은 제거한다(MarkItemDirty/MarkArrayDirty 포함).
	 * 원자성(총량 >= NumToConsume)은 호출자가 사전 검증해야 한다 — 부족분만큼만 부분 차감될 수 있다.
	 * 차감된 슬롯의 변경을 차감 순서대로 반환한다.
	 * NewStackCount 가 0 인 항목은 제거된 슬롯이다.
	 * 통지는 하지 않는다.
	 */
	TArray<FWxInventoryChangeResult> ConsumeByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume);

	const TArray<FWxInventoryEntry>& GetEntries() const;

private:
	UPROPERTY()
	TArray<FWxInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FWxInventoryList> : public TStructOpsTypeTraitsBase2<FWxInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * 인벤토리 정의 단위 합계 변경 브로드캐스트.
 * NewCount 는 해당 ItemDef 의 소유 총합, Delta 는 이번 변경분(양수/음수).
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryStackChanged, const UWxItemDefinition* /*ItemDef*/, int32 /*NewCount*/, int32 /*Delta*/);

/**
 * 슬롯(인스턴스) 단위 변경 브로드캐스트.
 * NewStackCount 는 해당 슬롯의 갱신 후 잔여 수량(제거 시 0), Delta 는 이번 변경분.
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventorySlotChanged, UWxItemInstance* /*Instance*/, int32 /*NewStackCount*/, int32 /*Delta*/);

/**
 * 충전형(Charges Fragment) 아이템의 인스턴스 충전량 변경 브로드캐스트.
 * NewCharges 는 갱신 후 충전 횟수, Delta 는 이번 변경분(사용 시 음수, 리필 시 양수).
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryChargeChanged, UWxItemInstance* /*Instance*/, int32 /*NewCharges*/, int32 /*Delta*/);

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnInventoryReady, UWxInventoryComponent* /*Inventory*/);

/**
 * PlayerController 에 부착되어 아이템 인스턴스의 생성·소멸·레플리케이션을 관장하는 컴포넌트.
 *
 * 권한(서버)에서만 Add/Consume 이 호출되어야 하며, FastArray 로 클라이언트에 동기화된다.
 *
 * 부착은 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 설정으로 한다(PlayerController 는 본 클래스를 모른다).
 * 등록하지 않으면 인벤토리가 조용히 없는 상태가 된다.
 * 시작 아이템은 Experience 의 Add Inventory Items 액션이 도착 신호(OnAnyInventoryReady)를 받아 지급한다 — 본 클래스는 목록을 갖지 않는다.
 */
UCLASS()
class WXINVENTORY_API UWxInventoryComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UWxInventoryComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReadyForReplication() override;
	//~ End UActorComponent interface

	/**
	 * 인벤토리가 쓸 수 있게 될 때마다 발행된다. 주입(서버)·복제 도착(클라) 어느 경로든 BeginPlay 로 수렴한다.
	 * 관찰자가 인벤토리보다 먼저 존재할 수 있어(HUD 뷰모델) 인스턴스가 아니라 클래스 차원에 둔다 — 구독자는 소유 액터로 자기 것인지 가린다.
	 */
	static FWxOnInventoryReady OnAnyInventoryReady;

	/**
	 * 인벤토리는 PlayerController 에 부착되므로, 액터가 Pawn 이면 소유 컨트롤러를 거쳐 조회한다.
	 * PlayerController 가 아닌 액터(또는 컨트롤러 미할당 폰) 는 nullptr.
	 */
	static UWxInventoryComponent* FindInventory(const AActor* Actor);

	/**
	 * 권한: ItemDef 를 StackCount 만큼 추가한다.
	 * Stackable Fragment 가 있으면 기존 엔트리에 MaxStack 한도까지 머지하고, 초과분은 새 엔트리들로 분할한다.
	 * Stackable Fragment 가 없으면 StackCount 만큼의 신규 엔트리(각 1개) 가 생성된다.
	 * 반환값은 첫 영향받은 인스턴스(머지된 기존 엔트리 또는 새로 만든 첫 엔트리).
	 * 실패 시 nullptr.
	 */
	UWxItemInstance* AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount = 1);

	/**
	 * 권한: 보상 항목 목록을 순서대로 지급한다(기본 지급 아이템 등).
	 * 빈(아이템 미지정) 항목은 무시되며, Item 은 지급 시점에 동기 로드된다.
	 */
	void GrantItems(const TArray<FWxItemRewardEntry>& Items);

	/**
	 * 권한: 특정 인스턴스 슬롯을 통째로 제거한다.
	 * 미구현: 현재 호출부가 0건이다(소비는 ConsumeItemsByDefinition 경로가 담당한다).
	 */
	void RemoveItemInstance(UWxItemInstance* ItemInstance);

	/**
	 * 권한: ItemDef 의 소유 수량을 NumToConsume 만큼 차감한다.
	 * 부족하면 false 반환하고 아무것도 차감하지 않는다(원자적).
	 * 0 이 된 슬롯은 제거한다.
	 * 같은 ItemDef 가 복수 엔트리로 분산돼 있어도 합산 차감이 가능하다.
	 */
	bool ConsumeItemsByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume);

	UWxItemInstance* FindFirstItemStackByDefinition(const UWxItemDefinition* ItemDef) const;

	int32 GetTotalItemCountByDefinition(const UWxItemDefinition* ItemDef) const;

	/** 인스턴스가 엔트리에 없으면 0. */
	int32 GetStackCountByInstance(const UWxItemInstance* Instance) const;

	TArray<UWxItemInstance*> GetAllItems() const;

	/**
	 * 입력이 아닌 경로(UI 클릭 등)의 진입점으로, 소유 폰의 UseItem 어빌리티를 AssetTag 로 발동한다 — 입력이 타는 것과 같은 경로다.
	 * 사용 가능 여부 판정과 차감은 어빌리티가 수행하므로 여기서는 검사하지 않는다.
	 *
	 * 소비 아이템은 에스트병 하나뿐이라 대상을 지목하지 않는다.
	 */
	bool RequestUseConsumable();

	/**
	 * Usable Fragment 보유 + (충전형이면 충전이 남은 인스턴스 존재) 이면 true.
	 * UseItemByDef 와 동일한 인스턴스 선택 기준을 공유한다.
	 */
	bool CanUseItemByDef(const UWxItemDefinition* ItemDef) const;

	/**
	 * 권한: Usable Fragment 를 가진 아이템을 1회 사용한다.
	 * 비 사용 아이템이면 false.
	 * 충전형(Charges Fragment)은 충전이 남은 첫 인스턴스의 충전량을 1 감소시키고(인벤토리 스택 유지), 그 외는 스택을 1 차감한다.
	 * 가용성·GE Spec 검증을 모두 통과한 뒤에만 차감하고, 차감 성공 후 GE를 소유 폰에 적용한다.
	 */
	bool UseItemByDef(const UWxItemDefinition* ItemDef);

	/**
	 * 권한: 충전형(Charges Fragment) 아이템 인스턴스의 충전량을 MaxCharges 로 회복한다(에스트병 체크포인트 리필).
	 * 충전형이 아니거나 인스턴스가 유효하지 않으면 false.
	 */
	bool RefillItemCharges(UWxItemInstance* Instance);

	/**
	 * 권한: 소유 중인 Equipment Fragment 아이템을 소유 폰의 UWxEquipmentComponent 에 장착 요청.
	 * 미소유 ItemDef 는 거부한다.
	 * 스택은 차감하지 않는다.
	 * ItemDef 가 nullptr 이면 장착 해제.
	 *
	 * 미구현: 현재 호출부가 0건이다(BlueprintCallable 도 아니라 BP 진입도 불가). 장비 경로 전체가 배선만 있고 트리거가 없는 상태다 — UWxEquipmentComponent 주석 참조.
	 */
	bool EquipItemByDef(const UWxItemDefinition* ItemDef);

	FWxOnInventoryStackChanged OnInventoryStackChanged;

	FWxOnInventorySlotChanged OnInventorySlotChanged;

	FWxOnInventoryChargeChanged OnInventoryChargeChanged;

	//~ 아래 3종은 List 복제 콜백/Instance OnRep/내부 변경 경로 전용 통지 진입점이다(외부 소비자 호출 금지, 비-BlueprintCallable).

	/** NewCount 는 내부에서 합계를 재계산한다. */
	void NotifyStackChangedFromList(const UWxItemDefinition* ItemDef, int32 Delta);

	void NotifySlotChangedFromList(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta);

	/** 서버(사용/리필)와 클라이언트(OnRep_CurrentCharges) 공통 진입. */
	void NotifyChargeChangedFromSource(UWxItemInstance* Instance, int32 NewCharges, int32 Delta);

private:
	/**
	 * ItemDef 의 사용 대상 인스턴스를 찾는다. 충전형(Charges)은 충전이 남은 첫 인스턴스, 그 외는 보유한 첫 인스턴스.
	 * UseItemByDef 와 CanUseItemByDef 가 동일한 선택 기준을 공유하기 위한 헬퍼다. 없으면 nullptr.
	 */
	UWxItemInstance* FindUsableInstance(const UWxItemDefinition* ItemDef) const;

	void RegisterReplicatedInstance(UWxItemInstance* Instance);

	void UnregisterReplicatedInstance(UWxItemInstance* Instance);

	UPROPERTY(Replicated)
	FWxInventoryList InventoryList;
};
