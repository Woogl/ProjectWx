# WxInventory — 코드 리뷰

> FastArray 기반 서버 권위 인벤토리로, 모듈 경계(`WxCore` 외 의존 0)와 코딩 규칙을 전부 지키고 복제 콜백의 델타 계산 의도가 주석에 남아 있는 건강한 모듈이다. 이번 리뷰는 `Build.cs`/`uplugin`과 공개 헤더 22개 전부, 그리고 인벤토리 컴포넌트·아이템 인스턴스·장비 컴포넌트·픽업·보상 라이브러리·StateTree 태스크의 cpp 로직을 읽고, 외부 호출부(`Source/WxGame` 의 아이템 사용 컴포넌트·뷰모델·캐릭터·게임모드)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 복제 콜백이 미해석 `Instance` 를 통지 없이 삼키면서 `LastObservedCount` 만 전진시킨다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:83`, `:85`, `:104`~`:107`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 `Entry.LastObservedCount = Entry.StackCount`(`:83`)를 **무조건 먼저** 수행하고, 그다음 `if (Entry.Instance)`(`:85`)로 통지 여부를 가른다. `UWxItemInstance` 는 `bReplicateUsingRegisteredSubObjectList` 로 복제되는 동적 서브오브젝트이고 컴포넌트 프로퍼티(=`InventoryList`)가 그 서브오브젝트 콘텐츠 블록보다 먼저 읽히므로, 신규 엔트리의 `Instance` 참조가 클라에서 아직 미해석(null)으로 들어올 수 있다. 이때 통지는 건너뛰는데 `LastObservedCount` 는 이미 최신값이라, 이후 참조가 해석되어 `PostReplicatedChange` 가 와도 `Delta = StackCount - LastObservedCount == 0`(`:104`)이라 `:107` 조건에서 또 걸러진다 — **해당 슬롯의 획득 통지가 영구 유실**된다. 인벤토리 목록의 유일한 소비자인 `UWxViewModel_Inventory` 는 `OnInventoryStackChanged` 를 받아야만 `RefreshAllItems()` 를 돌리므로(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp:115`), 데디케이티드 클라에서 픽업한 아이템이 UI에 끝내 안 뜨는 형태로 드러난다. 같은 배치 안에서 `NotifyStackChangedFromList` 가 부르는 `GetTotalItemCountByDefinition`(`:619`)도 미해석 엔트리를 건너뛰므로 `NewCount` 자체가 과소 집계된다.
- **제안**: `LastObservedCount` 전진을 통지와 같은 조건 안으로 옮겨(`Entry.Instance` 가 null 이면 갱신하지 않음) 참조 해석 후의 `PostReplicatedChange` 가 델타를 복구하게 한다. 더 확실히 하려면 정의 단위 합계는 콜백별 델타에 의존하지 말고 배치 종료 시점(`PostReplicatedReceive`)에 한 번 재계산해 발행한다.
- **확신도**: 중간 (통지 유실 코드 경로 자체는 확정적이나, 실제로 미해석 참조가 도달하는지는 네트워크 세션에서 재현 검증하지 않았다. 호스트/스탠드얼론에서는 발현되지 않는다)

### 2. 🟡 서버 권위 가드가 `check()` 라 Shipping 에서 통째로 사라진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:125`, `:268`, `:352`, `:386`, `:504`, `:569`, `:586`
- **범주**: 설계/구조
- **문제**: `AddEntry`/`AddItemDefinition`/`RemoveItemInstance`/`ConsumeItemsByDefinition`/`UseItemByDef`/`RefillItemCharges`/`EquipItemByDef` 가 전부 `check(GetOwner() && GetOwner()->HasAuthority())` 하나로만 권위를 강제한다. `check` 는 `DO_CHECK=0` 인 Shipping 에서 컴파일 아웃되므로, Shipping 에서는 비권위 호출이 막히지 않고 클라 로컬 인벤토리를 변조한 뒤 다음 FastArray 델타가 올 때까지 서버와 어긋난 상태로 남는다. 같은 모듈 안에서도 방식이 갈린다 — `UWxEquipmentComponent::EquipItem`(`WxEquipmentComponent.cpp:37`), `AWxItemPickup::LaunchInDirection`(`WxItemPickup.cpp:61`), `UWxRewardLibrary::GrantReward`(`WxRewardLibrary.cpp:16`)는 모두 `if (!HasAuthority()) return;` 형태의 실제 가드를 쓴다.
- **제안**: `ensure` + 조기 반환(또는 `if (!GetOwner()->HasAuthority()) return false;`)으로 바꿔 개발 빌드에서는 시끄럽게 알리되 Shipping 에서도 실제로 막는다.
- **확신도**: 높음 (다만 현재 호출부는 모두 스스로 권위를 가리고 있어 — `UWxItemUseComponent::HandleUseItemEvent`(`Source/WxGame/Inventory/WxItemUseComponent.cpp:74`), StateTree 두 태스크의 `Owner->HasAuthority()` — 즉시 발현되지는 않는다)

### 3. 🟡 `UseItemByDef` 가 인스턴스를 소멸시킨 뒤 그 인스턴스를 SourceObject 로 가진 GE 를 적용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:532`, `:549`, `:556`
- **범주**: 버그/정확성
- **문제**: `Context.AddSourceObject(SourceInstance)`(`:532`)로 Spec 을 만든 뒤, 비충전형 경로에서 `ConsumeItemsByDefinition(ItemDef, 1)`(`:549`)이 마지막 스택을 깎으면 `FWxInventoryList::ConsumeByDefinition` 이 엔트리를 제거해 `SourceInstance` 를 참조하던 유일한 UPROPERTY 가 사라진다. 그 상태로 `ApplyGameplayEffectSpecToSelf`(`:556`)를 호출하는데 `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 이므로, Instant 가 아닌 Duration/Infinite GE 라면 다음 GC 이후 SourceObject 가 null 이 된다. 헤더가 밝힌 목적("SourceObject 는 인스턴스 단위 데이터 추적이 가능하도록 ItemInstance 를 사용한다", `:530`)이 그대로 무효화되고, MMC/Execution/GameplayCue 가 SourceObject 로 아이템 정보를 되읽으면 조용히 실패한다.
- **제안**: 마지막 스택 소모로 인스턴스가 사라질 수 있는 경로는 SourceObject 를 수명이 안정적인 `UWxItemDefinition` 으로 두거나, 인스턴스를 유지해야 한다면 GE 적용을 차감보다 먼저 수행한다. 최소한 "이 경로의 GE 는 Instant 여야 한다"는 제약을 `UWxItemFragment_Usable::Effect`(`WxItemFragment.h:76`) 주석에 명시한다.
- **확신도**: 중간 (현재 유일한 소비 아이템이 충전형 에스트병이라 비충전형 경로가 아직 데이터로 존재하지 않을 수 있다)

### 4. 🟡 `AddItemDefinition` 이 엔트리 인덱스 순회 도중 델리게이트를 브로드캐스트해 재진입에 취약하다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:278`~`:302`, `:158`~`:164`
- **범주**: 버그/정확성
- **문제**: 머지 루프는 `InventoryList.GetEntries()` 로 내부 `Entries` 배열 참조를 잡은 채(`:278`) 매 반복마다 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 로 외부 구독자를 호출한다(`:295`~`:296`). 구독자가 그 콜백 안에서 인벤토리를 변경하면(획득 즉시 자동 사용·변환, 퀘스트 훅 등) 엔트리가 추가·삭제되어 `EntryIndex` 가 다른 슬롯을 가리키게 되고, 다음 반복의 `AddToEntryStack(EntryIndex, ...)` 이 엉뚱한 슬롯에 수량을 얹는다 — `AddToEntryStack` 은 인덱스 유효성을 따로 검사하지 않고 `Entries[EntryIndex]`(`:160`)로 바로 들어가는데, `TArray` 자체의 범위 검사도 Shipping 에서는 컴파일 아웃된다. 현재 구독자(`UWxViewModel_Inventory`, `UWxViewModel_Item`)는 읽기 전용이라 발현되지 않지만, 델리게이트가 public 이라 구독자 추가만으로 조용히 깨진다.
- **제안**: 루프 안에서는 변경만 수행해 `(Instance, NewStackCount, Delta)` 를 로컬 배열에 모으고, 순회가 끝난 뒤 한 번에 브로드캐스트한다 — `ConsumeItemsByDefinition` 이 `FWxInventoryChangeResult` 로 이미 쓰는 패턴과 같다.
- **확신도**: 중간 (현재 구독자 구성에서는 잠재 위험이다)

### 5. 🟡 대량 지급 시 청크마다 전량 스캔 통지가 나가 O(N²) 이 된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:305`~`:323`, `:619`
- **범주**: 성능/안전
- **문제**: 신규 엔트리 생성 루프는 청크마다 `NotifyStackChangedFromList`(`:312`)를 부르고, 그 안에서 `GetTotalItemCountByDefinition`(`:619`)이 전체 엔트리를 다시 훑는다. 게다가 구독자 쪽 `UWxViewModel_Inventory::HandleStackChanged` 는 매 통지마다 `RefreshAllItems()`(전 슬롯 순회 + 슬롯당 기존 VM 선형 탐색 + 신규 `NewObject`)를 돌린다(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp:115`). Stackable Fragment 가 없으면 `MaxStack = 1` 이라 청크 수 = 수량이 되는데, 입력은 데이터에서 오고(`FWxItemRewardEntry::Quantity` 는 `ClampMin=1` 만 있고 상한이 없다, `WxRewardTableRow.h:24`) Stackable 부착을 잊은 재화·소재에 Quantity 500 을 넣으면 즉시 500개 인스턴스 + 500회 전량 스캔 + 500회 UI 전체 갱신이 된다.
- **제안**: 머지 루프와 생성 루프의 합계 델타를 모아 정의 단위 통지를 1회로 줄인다(내부 재계산도 1회). 더불어 Stackable 부재 + `StackCount > 1` 조합은 경고 로그를 남겨 데이터 실수를 드러낸다.
- **확신도**: 중간 (정상 데이터라면 발현되지 않는 데이터 사고 대비 성격이다)

### 6. 🟡 StateTree 보상·리필 태스크가 대상 플레이어를 0번으로 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:40`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:36`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 대상을 잡는다. 이 인덱스는 "로컬 플레이어 → 원격 플레이어" 순회 결과라 참가·이탈에 따라 바뀐다. 데디케이티드 서버에서 B 플레이어가 상자를 열거나 체크포인트를 밟아도 보상·에스트병 리필은 먼저 접속한 A 플레이어에게 간다. 바로 옆 픽업 경로가 `Interactor` 를 그대로 쓰는 것(`WxItemPickup.cpp:85`)과 대비되며, 태스크는 진짜 상호작용 주체를 알 수 있는 위치에 있는데도 버린다.
- **제안**: 대상 액터를 태스크 인스턴스 데이터의 바인딩 가능한 파라미터(`AActor* Target`)로 받아 상호작용을 일으킨 실제 플레이어를 흘려주고, `UWxInventoryComponent::FindInventory(Target)` 으로 해석한다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 두 태스크의 헤더 주석이 "로컬 플레이어(0번 컨트롤러)"를 명시적 전제로 적어두었고, `AWxEnemyCharacter` 의 처치 보상(`Source/WxGame/Character/WxEnemyCharacter.cpp:141`)과 `WxDialogue`·`WxQuest`·`WxUI` 의 StateTree 태스크들도 같은 방식을 쓴다. 즉 모듈 단독 결함이 아니라 프로젝트 전반의 싱글플레이 전제다)

### 7. 🟢 장비 경로 전체가 호출부 0건인 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:179`, `:234`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:27`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체 검색 결과 `UWxInventoryComponent::EquipItemByDef` 와 `RemoveItemInstance` 의 호출부가 0건이고, `UWxEquipmentComponent::EquipItem` 의 유일한 호출부가 `EquipItemByDef` 라 `EquippedItemDef` 는 항상 null 이다. 즉 `UWxEquipmentComponent` 의 GE 라이프사이클·`OnEquipVisualChanged` 방송 전체(헤더 71줄 + cpp 143줄)가 미검증 상태이며, `AWxCharacterBase` 가 `Source/WxGame/Character/WxCharacterBase.cpp:89` 에서 구독만 걸어둔 채 아무것도 받지 못한다. 헤더 주석이 예고한 "늦게 relevant 해진 클라가 초기 `OnRep_EquippedItemDef` 를 구독보다 먼저 받아 방송을 유실하는데 되물을 pull API 가 없다"는 문제도 경로를 여는 순간 바로 터진다.
- **제안**: 장비 기능을 당분간 안 쓸 거면 경로를 제거해 유지보수 표면을 줄이고, 열 계획이면 `EquipItemByDef` 호출부(UI 또는 어빌리티)와 함께 현재 장착 상태를 되묻는 getter(`GetEquippedItemDef`)를 같이 넣어 초기 복제 유실을 닫는다.
- **확신도**: 높음 (기능 미완이라는 사실 자체는 헤더에 명시돼 있어 인지된 상태다)

### 8. 🟢 `Usable` Fragment 에 `Effect` 가 비어 있으면 아이템이 조용히 소모만 된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:521`, `:554`
- **범주**: 버그/정확성
- **문제**: `if (Usable->Effect)`(`:521`)가 거짓이면 `TargetASC`/`Spec` 이 채워지지 않고, `if (TargetASC && Spec.IsValid())`(`:554`)도 건너뛴다. 그런데 충전 차감/스택 차감은 그대로 수행되고 `true` 를 반환하므로, `Usable` 만 붙이고 `Effect` 를 비워둔 데이터 실수는 "마시는 연출은 나오는데 아무 효과도 없고 개수만 준다"로 나타나며 로그도 남지 않는다. GE Spec 생성 실패(`:537`)는 차감 전에 `false` 로 거르는데, 그보다 흔한 미설정 케이스만 통과시키는 비대칭이다.
- **제안**: `Effect` 미설정을 실패로 보고 차감 전에 `false` 반환하거나, 최소한 경고 로그를 남긴다. "효과 없는 사용"이 의도된 조합이라면 `UWxItemFragment_Usable::Effect` 주석에 그렇게 적는다.
- **확신도**: 중간 (효과 없는 소모품이 의도된 조합일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/README.md`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, 교차 확인용 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`
- **규칙 준수 확인 결과**: 위반 없음. 22개 소스 전부 첫 줄 저작권 표기가 있고, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(Blueprint Function Library) 한 곳뿐이며, 람다·`FORCEINLINE`·인라인 정의가 없고 헤더 정의 4건(`UWxItemDefinition::FindFragmentByClass<T>`, `UWxItemInstance::FindFragmentByClass<T>`, StateTree 태스크 2종의 `GetInstanceDataType()`)은 모두 규칙 6 예외 사유가 주석으로 달려 있다. `Build.cs`/`uplugin` 의존과 실제 include 가 모두 `WxCore`(`WxInteractable.h`, `WxCollisionChannels.h`, `WxGameplayTags.h`)로 한정돼 플러그인 경계도 지켜진다. `Wx` 접두사도 전 타입에 붙어 있으며, 델리게이트 콜백은 이 모듈 안에 바인딩이 0건이라 `Handle` 접두사 규칙의 판정 대상이 없다(`OnRep_*` 는 엔진 RepNotify 규약이라 대상 아님). `override` 도 `EndPlay`/`OnInstanceCreated`/`PostEditChangeProperty`/`GetLifetimeReplicatedProps`/`ReadyForReplication`/`BeginPlay` 전부 `Super::` 를 호출한다.
- **미검토 / 한계**: 아이템/보상 데이터 에셋(`UWxItemDefinition`, `FWxRewardTableRow` DataTable)의 실제 내용은 열지 않아, 발견 3·5·8 이 현재 데이터에서 실제로 발현되는지는 확인하지 못했다. 발견 1 을 포함한 리플리케이션 지적은 코드 정적 분석과 엔진 액터 채널의 기록 순서(컴포넌트 프로퍼티 → 등록 서브오브젝트)에 대한 이해에 기반하며, 실제 네트워크 세션에서 재현 검증하지 않았다(빌드·에디터 실행 없이 읽기 전용으로만 리뷰). BP/WBP 내부 구조는 범위 밖이라 `AWxItemPickup` 파생 BP 의 설정(콜리전 프리셋·Niagara 오토액티베이트 등)은 보지 않았다. 이전 리뷰(`ba33d69e`)의 `AWxItemPickup::OnSaveRestored` 중복 지적은 WxSave 제거로 코드가 사라져 이번 목록에서 빠졌다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 22파일 — `/module-review`로 갱신*
