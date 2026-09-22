---
title: "아이템 소유·사용·StateTree 보상 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, inventory]
summary: "아이템 소유·사용·StateTree 보상 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 아이템 소유·사용·StateTree 보상 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs>)
- SHA-256: `efe240fcef0dd1aaf363653f9181ad304d1a9c49f9f48e97f921d4c40edff7c0`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"Core",
14: 			"CoreUObject",
15: 			"Engine",
16: 			"GameplayAbilities",
17: 			"GameplayTags",
18: 			"NetCore",
19: 			"StateTreeModule",
20: 			"WxCore",
21: 		});
22: 
23: 		PrivateDependencyModuleNames.AddRange(new string[]
24: 		{
25: 			"Niagara",
26: 		});
27: 	}
28: }
```
## Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h>)
- SHA-256: `00be7f10327142a22e695b4a3dfd324e4ef8f0f5837b17a41f9a12da12c4d5b1`
```text
...
45:  */
46: UCLASS(DisplayName = "Usable")
47: class WXINVENTORY_API UWxItemFragment_Usable : public UWxItemFragment
48: {
49: 	GENERATED_BODY()
50: 
51: public:
52: 	/** 사용 시 사용자(소유 폰)의 ASC에 적용할 GameplayEffect */
53: 	UPROPERTY(EditDefaultsOnly, Category = "Usable")
54: 	TSubclassOf<UGameplayEffect> Effect;
55: };
56: 
57: /**
58:  * 다크소울 에스트병 방식: 부착된 아이템은 인벤토리 스택이 아니라 인스턴스별 충전량으로 사용 가능 여부가 결정된다.
59:  * 사용 시 인벤토리 스택은 차감되지 않고(소진돼도 아이템은 인벤토리에 남는다) 충전량만 1 감소하며, 리필(UWxInventoryComponent::RefillItemCharges)로 MaxCharges 까지 회복한다.
60:  *
61:  * 기능 축은 Usable 과 직교하며, Usable 없이 단독 부착하면 사용 자체가 성립하지 않아 충전도 소모되지 않는다.
62:  */
63: UCLASS(DisplayName = "Refill")
64: class WXINVENTORY_API UWxItemFragment_Charges : public UWxItemFragment
65: {
66: 	GENERATED_BODY()
67: 
68: public:
69: 	/** 인스턴스 생성 시 이 값으로 가득 채워진다. */
70: 	UPROPERTY(EditDefaultsOnly, Category = "Charges", meta = (ClampMin = "1"))
71: 	int32 MaxCharges = 3;
72: 
73: 	/**
74: 	 * 인덱스 = 남은 충전 횟수(0=빈 상태, MaxCharges=가득)이므로 MaxCharges+1 개를 채운다.
75: 	 * 비어 있거나 현재 충전수에 해당하는 항목이 비어 있으면 UWxItemDefinition 의 기본 Icon 으로 폴백한다.
76: 	 */
77: 	UPROPERTY(EditDefaultsOnly, Category = "Charges", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
78: 	TArray<TSoftObjectPtr<UObject>> ChargeIcons;
79: 
80: 	//~ Begin UWxItemFragment interface
81: 	virtual void OnInstanceCreated(UWxItemInstance* Instance) const override;
82: 	//~ End UWxItemFragment interface
...
88:  */
89: UCLASS(DisplayName = "Stackable")
90: class WXINVENTORY_API UWxItemFragment_Stackable : public UWxItemFragment
91: {
92: 	GENERATED_BODY()
93: 
94: public:
95: 	/**
96: 	 * 상한을 둔 것은 동일 ItemDef 가 다수 슬롯에 분산되어도 합산이 int32 안에 들어오게 하기 위함이다.
97: 	 */
98: 	UPROPERTY(EditDefaultsOnly, Category = "Stackable", meta = (ClampMin = "1", ClampMax = "10000000"))
99: 	int32 MaxStack = 99;
100: };
101: 
102: /**
103:  * 다양한 스포너(보물 상자, 적 드랍 등) 가 아이템별로 지정된 픽업 액터를 스폰하면서도 동일한 픽업 BP 를 재사용해 아이템별로 다른 메시/이펙트를 출력하도록 한다.
104:  *
105:  * 본 Fragment 는 데이터만 기술한다 — 실제 스폰은 스포너(UWxRewardLibrary::GrantReward), 컴포넌트 세팅은 픽업 액터 측에서 수행한다.
106:  */
107: UCLASS(DisplayName = "Pickup")
108: class WXINVENTORY_API UWxItemFragment_Pickup : public UWxItemFragment
```
## Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp>)
- SHA-256: `22b1ce04abad06436349b13e5cc7374aee090239d450a5cb2af49746eb0e9a25`
```text
...
222: }
223: 
224: void UWxInventoryComponent::BeginPlay()
225: {
226: 	Super::BeginPlay();
227: 
228: 	// 도착 신호보다 앞서 지급해야 뷰모델이 전체 갱신 한 번으로 시작 아이템까지 읽는다.
229: 	if (GetOwner()->HasAuthority())
230: 	{
231: 		GrantItems(StartingItems);
232: 	}
233: 
234: 	OnAnyInventoryReady.Broadcast(this);
235: }
236: 
237: void UWxInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
238: {
239: 	// HasBegunPlay를 먼저 해제하여 종료 중인 자신에게 재연결하지 않도록 한다.
240: 	Super::EndPlay(EndPlayReason);
241: 	OnAnyInventoryEnded.Broadcast(this);
242: }
243: void UWxInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
244: {
245: 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
246: 
247: 	DOREPLIFETIME(ThisClass, InventoryList);
248: }
249: 
250: void UWxInventoryComponent::ReadyForReplication()
251: {
252: 	Super::ReadyForReplication();
253: 
254: 	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
255: 	{
256: 		if (UWxItemInstance* Instance = Entry.GetInstance())
257: 		{
258: 			AddReplicatedSubObject(Instance);
259: 		}
260: 	}
261: }
262: 
263: void UWxInventoryComponent::NotifyContentsChangedFromReplication()
264: {
265: 	OnInventoryContentsChanged.Broadcast();
266: }
267: 
268: UWxItemInstance* UWxInventoryComponent::AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount)
269: {
270: 	if (!ItemDef || StackCount <= 0)
271: 	{
272: 		return nullptr;
273: 	}
274: 
275: 	check(GetOwner() && GetOwner()->HasAuthority());
276: 
277: 	const UWxItemFragment_Stackable* Stackable = ItemDef->FindFragmentByClass<UWxItemFragment_Stackable>();
278: 	const int32 MaxStack = Stackable ? Stackable->MaxStack : 1;
279: 
280: 	int32 Remaining = StackCount;
281: 	UWxItemInstance* FirstAffected = nullptr;
282: 
283: 	if (MaxStack > 1)
284: 	{
285: 		const TArray<FWxInventoryEntry>& Entries = InventoryList.GetEntries();
286: 		for (int32 EntryIndex = 0; EntryIndex < Entries.Num() && Remaining > 0; ++EntryIndex)
...
345: 		if (const UWxItemDefinition* ItemDef = Entry.Item.LoadSynchronous())
346: 		{
347: 			AddItemDefinition(ItemDef, Entry.Quantity);
348: 		}
349: 	}
350: }
351: 
352: void UWxInventoryComponent::RemoveItemInstance(UWxItemInstance* ItemInstance)
353: {
354: 	if (!ItemInstance)
355: 	{
356: 		return;
357: 	}
358: 
359: 	check(GetOwner() && GetOwner()->HasAuthority());
360: 
361: 	int32 RemovedStackCount = 0;
362: 	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
363: 	{
364: 		if (Entry.GetInstance() == ItemInstance)
365: 		{
...
495: }
496: 
497: bool UWxInventoryComponent::CanUseItemByDef(const UWxItemDefinition* ItemDef) const
498: {
499: 	return ItemDef
500: 		&& ItemDef->FindFragmentByClass<UWxItemFragment_Usable>()
501: 		&& FindUsableInstance(ItemDef) != nullptr;
502: }
503: 
504: bool UWxInventoryComponent::UseItemByDef(const UWxItemDefinition* ItemDef)
505: {
506: 	if (!ItemDef)
507: 	{
508: 		return false;
509: 	}
510: 
511: 	check(GetOwner() && GetOwner()->HasAuthority());
512: 
513: 	const UWxItemFragment_Usable* Usable = ItemDef->FindFragmentByClass<UWxItemFragment_Usable>();
514: 	if (!Usable)
515: 	{
516: 		return false;
517: 	}
518: 
519: 	UWxItemInstance* SourceInstance = FindUsableInstance(ItemDef);
520: 	if (!SourceInstance)
521: 	{
522: 		return false;
...
545: 	}
546: 
547: 	const UWxItemFragment_Charges* Charges = ItemDef->FindFragmentByClass<UWxItemFragment_Charges>();
548: 	if (Charges)
549: 	{
550: 		const int32 OldCharges = SourceInstance->GetCurrentCharges();
551: 		SourceInstance->SetCurrentCharges(OldCharges - 1);
552: 		const int32 NewCharges = SourceInstance->GetCurrentCharges();
553: 		NotifyChargeChangedFromSource(SourceInstance, NewCharges, NewCharges - OldCharges);
554: 	}
555: 	else if (!ConsumeItemsByDefinition(ItemDef, 1))
556: 	{
557: 		return false;
558: 	}
559: 
560: 	if (TargetASC && Spec.IsValid())
561: 	{
562: 		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
563: 	}
564: 
565: 	return true;
...
626: 	}
627: 
628: 	const UWxItemFragment_Charges* Charges = ItemDef->FindFragmentByClass<UWxItemFragment_Charges>();
629: 
630: 	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
631: 	{
632: 		UWxItemInstance* SlotInstance = Entry.GetInstance();
633: 		if (!SlotInstance || SlotInstance->GetItemDef() != ItemDef)
634: 		{
635: 			continue;
636: 		}
637: 		if (Charges && SlotInstance->GetCurrentCharges() <= 0)
638: 		{
639: 			continue;
640: 		}
641: 
642: 		return SlotInstance;
643: 	}
644: 
645: 	return nullptr;
646: }
```
## Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp>)
- SHA-256: `e1a654930a4b796985227c9241d2ff81c512a6ae3cd30abd65c2782148142aab`
```text
...
46: 	UseItemEventHandle = ASC->GenericGameplayEventCallbacks
47: 		.FindOrAdd(WxGameplayTags::Event_UseItem)
48: 		.AddUObject(this, &UWxItemUseComponent::HandleUseItemEvent);
49: }
50: 
51: void UWxItemUseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
52: {
53: 	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
54: 	{
55: 		if (FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(WxGameplayTags::Event_UseItem))
56: 		{
57: 			EventDelegate->Remove(UseItemEventHandle);
58: 		}
59: 	}
60: 
61: 	PendingItemDefinition = nullptr;
62: 	AbilitySystemComponent.Reset();
63: 	UseItemEventHandle.Reset();
64: 
65: 	Super::EndPlay(EndPlayReason);
66: }
67: 
68: void UWxItemUseComponent::HandleUseItemEvent(const FGameplayEventData* Payload)
69: {
70: 	AActor* Owner = GetOwner();
71: 	if (!Owner || !Owner->HasAuthority() || !Payload || !PendingItemDefinition)
72: 	{
73: 		return;
74: 	}
75: 
76: 	const UWxAnimNotify_UseItem* UseItemNotify = Cast<UWxAnimNotify_UseItem>(Payload->OptionalObject.Get());
77: 	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Payload->OptionalObject2.Get());
78: 	if (!UseItemNotify || !Mesh || Mesh->GetOwner() != Owner)
79: 	{
80: 		return;
81: 	}
82: 
83: 	UWxItemDefinition* ItemDefinition = PendingItemDefinition;
84: 	PendingItemDefinition = nullptr;
85: 
86: 	const APawn* Pawn = Cast<APawn>(Owner);
```
## Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp>)
- SHA-256: `e947693b7466cb9a5295a665792aa86e987aaf287e329a0469135c8a265798c5`
```text
...
19: }
20: 
21: EStateTreeRunStatus FWxStateTreeTask_GiveRewards::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
22: {
23: 	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
24: 	if (bInitialEntry)
25: 	{
26: 		return EStateTreeRunStatus::Succeeded;
27: 	}
28: 
29: 	AActor* Owner = Cast<AActor>(Context.GetOwner());
30: 	if (!Owner || !Owner->HasAuthority())
31: 	{
32: 		return EStateTreeRunStatus::Succeeded;
33: 	}
34: 
35: 	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
36: 
37: 	const FTransform OwnerTransform = Owner->GetActorTransform();
38: 	const FTransform SpawnTransform(OwnerTransform.GetRotation(), OwnerTransform.TransformPosition(Instance.SpawnOffset));
39: 
```
## Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp>)
- SHA-256: `6983934f44f83f56fe2c49145082d243dd28d7227fd97040388b6f825c76f470`
```text
...
19: }
20: 
21: EStateTreeRunStatus FWxStateTreeTask_RefillItemCharges::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
22: {
23: 	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
24: 	if (bInitialEntry)
25: 	{
26: 		return EStateTreeRunStatus::Succeeded;
27: 	}
28: 
29: 	AActor* Owner = Cast<AActor>(Context.GetOwner());
30: 	if (!Owner || !Owner->HasAuthority())
31: 	{
32: 		return EStateTreeRunStatus::Succeeded;
33: 	}
34: 
35: 	// 대상 선택은 보상 직접 지급 경로와 같은 전제다.
36: 	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
37: 	UWxInventoryComponent* Inventory = PlayerController ? PlayerController->FindComponentByClass<UWxInventoryComponent>() : nullptr;
38: 	if (!Inventory)
39: 	{
40: 		return EStateTreeRunStatus::Succeeded;
41: 	}
42: 
43: 	for (UWxItemInstance* Item : Inventory->GetAllItems())
44: 	{
45: 		Inventory->RefillItemCharges(Item);
46: 	}
47: 
48: 	return EStateTreeRunStatus::Succeeded;
49: }
```
