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

	/** 권한: 수량을 더하고 복제 대상으로 표시한다(MarkItemDirty). */
	void AddToEntryStack(int32 EntryIndex, int32 Amount);

	/** 권한: 총량 검증과 통지는 호출자 몫이라 부족하면 있는 만큼만 부분 차감되며, 반환의 NewStackCount 가 0 인 항목은 제거된 슬롯이다. */
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

/** NewCount 는 해당 ItemDef 의 슬롯 단위가 아닌 소유 총합이다. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryStackChanged, const UWxItemDefinition* /*ItemDef*/, int32 /*NewCount*/, int32 /*Delta*/);

/** 충전형(Charges Fragment) 아이템의 인스턴스 단위 충전량 변경이다. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryChargeChanged, UWxItemInstance* /*Instance*/, int32 /*NewCharges*/, int32 /*Delta*/);

DECLARE_MULTICAST_DELEGATE(FWxOnInventoryContentsChanged);

/** AWxPlayerController 의 기본 서브오브젝트이며, Add/Consume 은 권한(서버)에서만 호출하고 FastArray 로 클라이언트에 동기화된다. */
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

	/** 권한: Stackable Fragment 가 있으면 기존 엔트리에 MaxStack 까지 머지하고 초과분은 새 엔트리로 나누며(없으면 1개씩), 첫 영향받은 인스턴스를 반환한다. */
	UWxItemInstance* AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount = 1);

	/** 권한: 빈(아이템 미지정) 항목은 무시되며, Item 은 지급 시점에 동기 로드된다. */
	void GrantItems(const TArray<FWxItemRewardEntry>& Items);

	/** 권한: 부족하면 아무것도 차감하지 않고 false 를 반환하며(원자적), 여러 엔트리에 분산돼 있어도 합산 차감한다. */
	bool ConsumeItemsByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume);

	UWxItemInstance* FindFirstItemStackByDefinition(const UWxItemDefinition* ItemDef) const;

	int32 GetTotalItemCountByDefinition(const UWxItemDefinition* ItemDef) const;

	/** 인스턴스가 엔트리에 없으면 0. */
	int32 GetStackCountByInstance(const UWxItemInstance* Instance) const;

	TArray<UWxItemInstance*> GetAllItems() const;

	/** 쓸 수 있는 소비 아이템(Usable Fragment, 충전형이면 충전이 남은 것)이 있으면 true. */
	bool CanUseConsumable() const;

	/** 권한: 충전형은 충전량을, 그 외는 스택을 1 줄이되 가용성·GE Spec 검증을 모두 통과한 뒤에만 차감하고 GE 를 소유 폰에 적용한다. */
	bool UseConsumable();

	/** 권한: 충전형(Charges Fragment) 인스턴스의 충전량을 MaxCharges 로 회복하며, 충전형이 아니면 false. */
	bool RefillItemCharges(UWxItemInstance* Instance);

	/** 권한: 체크포인트·부활에서 소유한 충전형 아이템을 모두 회복한다. */
	void RefillAllItemCharges();

	FWxOnInventoryStackChanged OnInventoryStackChanged;

	FWxOnInventoryChargeChanged OnInventoryChargeChanged;

	/** 복제된 목록/정의의 현재 상태를 다시 읽으라는 통지. 획득 이벤트로 해석하지 않는다. */
	FWxOnInventoryContentsChanged OnInventoryContentsChanged;
	void NotifyContentsChangedFromReplication();

	//~ 아래 2종은 List 복제 콜백/Instance OnRep/내부 변경 경로 전용 통지 진입점이다(외부 소비자 호출 금지, 비-BlueprintCallable).

	/** NewCount 는 내부에서 합계를 재계산한다. */
	void NotifyStackChangedFromList(const UWxItemDefinition* ItemDef, int32 Delta);

	/** 서버(추가/사용/리필)와 클라이언트(OnRep_CurrentCharges) 공통 진입. */
	void NotifyChargeChangedFromSource(UWxItemInstance* Instance, int32 NewCharges, int32 Delta);

private:

	/** 소비 아이템은 에스트병 하나뿐이라 Usable Fragment 를 가진 첫 인스턴스(충전형이면 충전이 남은 것)를 고른다. */
	UWxItemInstance* FindConsumableInstance() const;

	void RegisterReplicatedInstance(UWxItemInstance* Instance);

	void UnregisterReplicatedInstance(UWxItemInstance* Instance);

	UPROPERTY(Replicated)
	FWxInventoryList InventoryList;

	/** BeginPlay 에서 권한 측이 한 번 지급한다. 빈 항목은 무시된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (TitleProperty = "{Item} x{Quantity}"))
	TArray<FWxItemRewardEntry> StartingItems;
};
