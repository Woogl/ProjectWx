// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/WxRewardTableRow.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "WxInventoryComponent.generated.h"

class UWxItemDefinition;
class UWxItemInstance;
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

/** 함수 결과 전용이라 USTRUCT 이 아니며, 담는 포인터도 GC 추적이 필요 없는 transient 다. */
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
	void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters);
	//~ End FFastArraySerializer

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

	/** 권한: Fragment 의 OnInstanceCreated 가 호출된다. */
	UWxItemInstance* AddEntry(const UWxItemDefinition* ItemDef, int32 StackCount);

	/** 권한: 갱신 후 수량을 반환한다(MarkItemDirty 포함). */
	int32 AddToEntryStack(int32 EntryIndex, int32 Amount);

	/**
	 * 권한: ItemDef 를 NumToConsume 만큼 슬롯 순서대로 차감하고 0 이 된 슬롯은 제거한다(MarkItemDirty/MarkArrayDirty 포함).
	 * 총량 검증과 통지는 호출자 몫이라, 부족하면 있는 만큼만 부분 차감된다.
	 * 차감된 슬롯의 변경을 차감 순서대로 반환하며, NewStackCount 가 0 인 항목은 제거된 슬롯이다.
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

DECLARE_MULTICAST_DELEGATE(FWxOnInventoryContentsChanged);

/**
 * AWxPlayerController 의 기본 서브오브젝트로 붙어 아이템 인스턴스의 생성·소멸·레플리케이션을 관장한다.
 * 권한(서버)에서만 Add/Consume 이 호출되어야 하며, FastArray 로 클라이언트에 동기화된다.
 */
UCLASS()
class WXGAME_API UWxInventoryComponent : public UActorComponent
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
	 * 권한: ItemDef 를 StackCount 만큼 추가한다.
	 * Stackable Fragment 가 있으면 기존 엔트리에 MaxStack 한도까지 머지하고, 초과분은 새 엔트리들로 분할한다.
	 * Stackable Fragment 가 없으면 StackCount 만큼의 신규 엔트리(각 1개) 가 생성된다.
	 * 반환값은 첫 영향받은 인스턴스(머지된 기존 엔트리 또는 새로 만든 첫 엔트리)이고, 실패 시 nullptr.
	 */
	UWxItemInstance* AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount = 1);

	/** 권한: 빈(아이템 미지정) 항목은 무시되며, Item 은 지급 시점에 동기 로드된다. */
	void GrantItems(const TArray<FWxItemRewardEntry>& Items);

	/**
	 * 권한: ItemDef 의 소유 수량을 NumToConsume 만큼 차감한다.
	 * 부족하면 false 반환하고 아무것도 차감하지 않는다(원자적).
	 * 같은 ItemDef 가 복수 엔트리로 분산돼 있어도 합산 차감하고, 0 이 된 슬롯은 제거한다.
	 */
	bool ConsumeItemsByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume);

	UWxItemInstance* FindFirstItemStackByDefinition(const UWxItemDefinition* ItemDef) const;

	int32 GetTotalItemCountByDefinition(const UWxItemDefinition* ItemDef) const;

	/** 인스턴스가 엔트리에 없으면 0. */
	int32 GetStackCountByInstance(const UWxItemInstance* Instance) const;

	TArray<UWxItemInstance*> GetAllItems() const;

	/** 쓸 수 있는 소비 아이템(Usable Fragment, 충전형이면 충전이 남은 것)이 있으면 true. */
	bool CanUseConsumable() const;

	/**
	 * 권한: 소비 아이템을 1회 사용한다. 없으면 false.
	 * 충전형(Charges Fragment)은 충전량을 1 감소시키고(인벤토리 스택 유지), 그 외는 스택을 1 차감한다.
	 * 가용성·GE Spec 검증을 모두 통과한 뒤에만 차감하고, 차감 성공 후 GE를 소유 폰에 적용한다.
	 */
	bool UseConsumable();

	/**
	 * 권한: 충전형(Charges Fragment) 아이템 인스턴스의 충전량을 MaxCharges 로 회복한다(에스트병 체크포인트 리필).
	 * 충전형이 아니거나 인스턴스가 유효하지 않으면 false.
	 */
	bool RefillItemCharges(UWxItemInstance* Instance);

	/** 권한: 체크포인트·부활에서 소유한 충전형 아이템을 모두 회복한다. */
	void RefillAllItemCharges();

	FWxOnInventoryStackChanged OnInventoryStackChanged;

	FWxOnInventorySlotChanged OnInventorySlotChanged;

	FWxOnInventoryChargeChanged OnInventoryChargeChanged;

	/** 복제된 목록/정의의 현재 상태를 다시 읽으라는 통지. 획득 이벤트로 해석하지 않는다. */
	FWxOnInventoryContentsChanged OnInventoryContentsChanged;
	void NotifyContentsChangedFromReplication();

	//~ 아래 3종은 List 복제 콜백/Instance OnRep/내부 변경 경로 전용 통지 진입점이다(외부 소비자 호출 금지, 비-BlueprintCallable).

	/** NewCount 는 내부에서 합계를 재계산한다. */
	void NotifyStackChangedFromList(const UWxItemDefinition* ItemDef, int32 Delta);

	void NotifySlotChangedFromList(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta);

	/** 서버(추가/사용/리필)와 클라이언트(OnRep_CurrentCharges) 공통 진입. */
	void NotifyChargeChangedFromSource(UWxItemInstance* Instance, int32 NewCharges, int32 Delta);

private:

	/**
	 * 소비 아이템은 에스트병 하나뿐이라 Usable Fragment를 가진 첫 인스턴스를 고른다. 충전형이면 충전이 남은 것만 고른다.
	 * UseConsumable 과 CanUseConsumable 이 동일한 선택 기준을 공유하기 위한 헬퍼다. 없으면 nullptr.
	 */
	UWxItemInstance* FindConsumableInstance() const;

	void RegisterReplicatedInstance(UWxItemInstance* Instance);

	void UnregisterReplicatedInstance(UWxItemInstance* Instance);

	UPROPERTY(Replicated)
	FWxInventoryList InventoryList;

	/** BeginPlay 에서 권한 측이 한 번 지급한다. 빈 항목은 무시된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (TitleProperty = "{Item} x{Quantity}"))
	TArray<FWxItemRewardEntry> StartingItems;
};
