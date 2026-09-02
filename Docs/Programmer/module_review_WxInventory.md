# WxInventory — 코드 리뷰

> 데이터 주도 아이템 정의 + FastArray 서버 권위 인벤토리라는 구조 자체는 견고하고, 코딩·모듈 규칙 위반은 이번 리뷰에서 한 건도 없다(Copyright 첫 줄·`Wx` 접두사·람다 0건·인라인 정의 0건·`BlueprintCallable` 은 `UWxRewardLibrary` 한 곳뿐·override 의 `Super::` 호출 모두 정상, 의존성은 `WxCore` + 엔진 플러그인만). 남은 문제는 클라이언트 통지 수렴, 권위 가드 스타일, 그리고 배선되지 않은 채 남은 장비 경로에 몰려 있다. 이번 리뷰는 22개 소스 전부를 훑고 인벤토리 컴포넌트·복제 콜백·보상/픽업 경로의 cpp 를 라인 단위로 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 복제 콜백이 미해석 `Instance` 를 통지 없이 삼키면서 `LastObservedCount` 만 전진시킨다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:80-91`, `:101-113`
- **범주**: 버그/정확성
- **문제**: `UActorChannel` 은 컴포넌트의 프로퍼티(=`InventoryList`)를 먼저 쓰고 등록 서브오브젝트(`UWxItemInstance`)를 뒤에 쓴다. 따라서 첫 수신 시 `PostReplicatedAdd` 시점에는 인스턴스가 클라에 아직 생성되지 않아 `Entry.Instance` 가 미해석(null)일 수 있다. 그런데 `Entry.LastObservedCount = Entry.StackCount;`(:83)는 `Instance` 유무와 무관하게 먼저 전진하고, 통지는 `if (Entry.Instance)` 에 막혀 나가지 않는다. 이후 참조가 해석돼 `PostReplicatedChange` 가 와도 `Delta = StackCount - LastObservedCount == 0`(:104,:107) 이라 다시 통지되지 않는다 — 해당 슬롯은 클라 뷰모델에 영원히 나타나지 않는다.
- **제안**: `Instance` 가 null 이면 `LastObservedCount` 를 전진시키지 않는다(생성자 초기값 `INDEX_NONE` 유지). 그러면 참조 해석 후의 `PostReplicatedChange` 에서 Delta 가 전체 수량으로 잡혀 자연히 복구된다.
- **확신도**: 중간

### 2. 🟡 서버 권위 가드가 `check()` 라 Shipping 에서 통째로 사라진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:268`, `:352`, `:386`, `:504`, `:568`, `:585`, `:125`
- **범주**: 설계/구조
- **문제**: 인벤토리 변경 진입점 6곳이 모두 `check(GetOwner() && GetOwner()->HasAuthority())` 로 권위를 강제한다. `check` 는 Shipping(`DO_CHECK=0`)에서 제거되므로, 실제 출시 빌드에서는 권위 계약이 아무것도 강제되지 않고 클라가 로컬 상태를 조용히 변조·디싱크시킨다. 반대로 Development 에서는 잘못된 호출 하나가 즉시 크래시다. 같은 모듈 안에서도 `UWxEquipmentComponent::EquipItem`(`Private/Inventory/WxEquipmentComponent.cpp:37`)과 `AWxItemPickup::LaunchInDirection`(`Private/Items/WxItemPickup.cpp:61`)은 조기 `return` 으로 우아하게 거부해 스타일이 갈린다.
- **제안**: 변경 진입점을 `if (!HasAuthority()) { return false/nullptr; }` + `ensure` 로그로 통일해 모듈 내 가드 방식을 일치시킨다.
- **확신도**: 중간(개발 중 fail-fast 를 의도한 설계일 수 있음)

### 3. 🟡 `UseItemByDef` 가 인스턴스를 인벤토리에서 떼어낸 뒤 그 인스턴스를 SourceObject 로 가진 GE 를 적용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:529-537`, `:548`, `:553-556`
- **범주**: 버그/정확성
- **문제**: `Context.AddSourceObject(SourceInstance)`(:531)로 Spec 을 먼저 만들고, 비-충전형 경로에서 `ConsumeItemsByDefinition(ItemDef, 1)`(:548)이 마지막 스택을 소진하면 엔트리가 제거되어 `SourceInstance` 의 유일한 UPROPERTY 참조가 사라진다. 그 뒤 `ApplyGameplayEffectSpecToSelf`(:555). `FGameplayEffectContext::SourceObject` 는 약참조라, Duration/Infinite GE 라면 다음 GC 이후 SourceObject 가 null 이 되어 효과 측의 인스턴스별 데이터 조회가 끊긴다(Instant GE 는 즉시 실행이라 무해).
- **제안**: SourceObject 로 `UWxItemInstance` 대신 `UWxItemDefinition` 을 싣거나, 소비 대상 인스턴스를 GE 적용이 끝날 때까지 강참조로 붙들어 둔다.
- **확신도**: 중간(현재 소비 아이템이 Instant GE 뿐이면 표면화되지 않음)

### 4. 🟡 StateTree 보상·리필 태스크가 대상 플레이어를 0번으로 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:40`, `Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:36`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 고른다. 모듈 전체가 서버 권위 + FastArray 복제로 설계돼 있는데, 정작 보상 직접 지급과 에스트병 리필은 항상 0번 컨트롤러에게만 간다. 두 번째 플레이어가 상자를 열거나 화톳불을 쓰면 보상·리필이 남에게 흘러간다.
- **제안**: 기믹을 발동한 액터(상호작용 Interactor)를 StateTree 파라미터로 바인딩해 대상으로 넘긴다. `AWxDevice` 계열은 이미 Interactor 를 알고 있으므로 배선만 하면 된다.
- **확신도**: 중간(현재 1인 플레이 전제의 의도된 단순화일 수 있음 — `RefillItemCharges` 헤더 주석이 "로컬 플레이어(0번 컨트롤러)"라고 명시)

### 5. 🟡 비-Stackable 대량 지급이 상한 없이 N개 인스턴스를 만들고 청크마다 전량 재스캔한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:305-323`, `:618`
- **범주**: 성능/안전
- **문제**: Stackable Fragment 가 없으면 `MaxStack = 1` 이라 `AddItemDefinition(Def, N)` 이 `NewObject` + `AddReplicatedSubObject` 를 N회 수행한다(:305-323). 게다가 청크마다 호출되는 `NotifyStackChangedFromList` 가 매번 `GetTotalItemCountByDefinition`(:618)으로 전체 엔트리를 훑어 총 O(N²) 이 된다. `GrantReward`/`GrantItems` 는 DataTable 의 `Quantity` 를 그대로 넘기므로, 디자이너가 Stackable 없는 아이템에 큰 수량을 넣으면 지급 한 번에 프레임이 멈추고 복제 서브오브젝트가 폭증한다. 코드에는 어떤 상한도 없다.
- **제안**: `AddItemDefinition` 진입부에 `StackCount` 상한(또는 Stackable 부재 시 수량 클램프)을 두고, 통지는 루프 밖에서 합산 델타 1회로 모은다.
- **확신도**: 높음

### 6. 🟡 `AddItemDefinition` 이 엔트리 인덱스 순회 도중 델리게이트를 브로드캐스트한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:279-302`
- **범주**: 설계/구조
- **문제**: 머지 루프가 `const TArray<FWxInventoryEntry>& Entries`(:278)를 인덱스로 순회하면서 루프 안에서 `NotifySlotChangedFromList`/`NotifyStackChangedFromList`(:295-296)를 쏜다. 구독자가 그 통지에 반응해 다시 인벤토리를 변경하면(예: UI 가 자동 소비를 트리거) 엔트리 인덱스가 어긋나 잘못된 슬롯에 머지된다. 메모리 안전 문제는 아니지만(참조는 배열 객체 자체) 논리적 재진입 결함이다.
- **제안**: 순회 중에는 변경만 하고, 통지는 루프 종료 후 수집된 결과를 한 번에 발행한다(`ConsumeItemsByDefinition` 이 이미 쓰는 `FWxInventoryChangeResult` 패턴과 동일하게).
- **확신도**: 낮음(현재 구독자가 표시 전용 뷰모델뿐이면 표면화되지 않음)

### 7. 🟡 픽업 비주얼을 게임스레드에서 동기 로드하고, 데디케이티드 서버에서도 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:56`, `:116-137`
- **범주**: 성능/안전
- **문제**: `SetItemDef` 가 곧바로 `ApplyPickupVisual()` 을 부르고(:56), 거기서 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous`(:129,:134) 한다. 드랍은 전투 중 다발적으로 발생하므로 첫 드랍마다 게임스레드 히치가 난다. 또 `SetItemDef` 는 서버 스폰 경로에서 호출되는데 데디케이티드 서버에는 비주얼 에셋이 전혀 필요 없다(클라는 `OnRep_ItemDef` 로 따로 로드한다). 헤더 주석은 "서버 권한에서만 호출"이라 하지만 코드에는 권한 가드가 없다.
- **제안**: `ApplyPickupVisual` 초입에 `IsRunningDedicatedServer()` 또는 `IsNetMode(NM_DedicatedServer)` 게이트를 두고, 드랍이 잦은 아이템은 사전 프리로드(보상 테이블 로드 시 비동기 요청)로 히치를 없앤다.
- **확신도**: 중간

### 8. 🟢 장비 경로 전체와 `RemoveItemInstance` 가 호출부 0건인 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:583`(`EquipItemByDef`), `:345`(`RemoveItemInstance`), `Public/Inventory/WxEquipmentComponent.h` 전체
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 는 프로젝트 전역에서 호출부가 없고 `BlueprintCallable` 도 아니라 BP 진입도 불가하다. 그 유일한 소비자인 `UWxEquipmentComponent::EquipItem` 도 마찬가지다. 그런데 `AWxCharacterBase` 는 모든 캐릭터에 `EquipmentComponent` 를 CDO 로 붙이고(`Source/WxGame/Character/WxCharacterBase.cpp:43`) `OnEquipVisualChanged` 를 구독까지 한다(:89) — 항상 null 인 `EquippedItemDef` 를 복제하는 컴포넌트가 전 캐릭터에 붙어 있는 셈이다. `RemoveItemInstance` 도 동일하게 호출부 0건이다. 헤더 주석이 지적한 late-join RepNotify 유실(현재 상태를 되물을 pull API 부재)도 경로를 닫을 때 함께 처리해야 한다.
- **제안**: 장비 경로를 실제로 배선하거나(트리거 + pull API 추가), 당분간 쓰지 않을 것이면 컴포넌트 부착까지 걷어내 복제 비용과 오해를 없앤다. `RemoveItemInstance` 는 삭제.
- **확신도**: 높음

### 9. 🟢 `Usable` Fragment 에 `Effect` 가 비어 있으면 아이템이 조용히 소모만 된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:520-538`, `:540-551`
- **범주**: 버그/정확성
- **문제**: `if (Usable->Effect)`(:520) 가 false 면 `TargetASC`/`Spec` 이 비어 있는 채로 통과하고, 그대로 충전 차감 또는 스택 차감이 실행된다(:540-551). 즉 GE 를 지정하지 않은 Usable 아이템은 아무 효과 없이 사라지며 `UseItemByDef` 는 `true` 를 반환한다 — 데이터 오설정이 런타임에 조용히 아이템 손실로 나타난다.
- **제안**: `Effect` 미지정이면 `false` 로 조기 반환하거나, 최소한 경고 로그를 남긴다.
- **확신도**: 높음

### 10. 🟢 `GrantReward` 가 `World` 널 검사와 스폰 후 유효성 검사를 하지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:38`, `:70`, `:77-79`
- **범주**: 버그/정확성
- **문제**: `SourceActor->GetWorld()`(:38) 결과를 검사 없이 `World->SpawnActorDeferred`(:70)에 쓴다. 또 `FinishSpawning`(:77) 중 BP BeginPlay 가 픽업을 파괴할 수 있는데, 그 뒤 `SpawnedPickup->LaunchInDirection`(:79)을 무조건 호출한다.
- **제안**: `World` 널 가드 추가, `LaunchInDirection` 앞에 `IsValid(SpawnedPickup)` 검사 추가.
- **확신도**: 중간(런타임에서 `World` 가 null 일 확률은 낮음)

### 11. 🟢 `UWxItemFragment_Charges` 의 에디터 표시 이름이 "Refill" 이다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:85`
- **범주**: 중복/복잡도
- **문제**: 클래스명·README·코드 전반이 "Charges" 로 부르는데 `UCLASS(DisplayName = "Refill")` 만 "Refill" 이다. 디자이너가 Fragment 목록에서 고를 때 이름이 매칭되지 않아 혼동을 낳는다(다른 5종은 모두 클래스명과 일치).
- **제안**: `DisplayName = "Charges"` 로 통일.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Public/Inventory/WxInventoryComponent.h`, `Private/Items/WxItemPickup.cpp`, `Private/WxRewardLibrary.cpp`, `Private/Inventory/WxEquipmentComponent.cpp`, `Private/Items/WxItemInstance.cpp`
- **훑은 파일**: `Public/Items/WxItemFragment.h`, `Private/Items/WxItemFragment.cpp`, `Public/Items/WxItemDefinition.h`, `Private/Items/WxItemDefinition.cpp`, `Public/Items/WxRewardTableRow.h`, `Private/Items/WxRewardTableRow.cpp`, `Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Public/WxInventoryModule.h`, `Private/WxInventoryModule.cpp`, `WxInventory.Build.cs`, `WxInventory.uplugin`
- **교차 확인**: 호출부 검증을 위해 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCore/.../WxInteractable.h` 를 함께 읽었다
- **미검토 / 한계**:
  - 발견 1의 서브오브젝트 도착 순서·미해석 참조 재통지 경로는 엔진 `UActorChannel`/`FFastArraySerializer` 소스를 직접 열지 않고 구조로만 추론했다. PIE 클라이언트에서 실제 재현 확인이 필요하다.
  - `UWxItemDefinition`/`UWxItemFragment` 를 사용하는 `.uasset`(아이템 정의, 보상 DataTable, 픽업 BP)의 실제 데이터는 확인하지 않았다 — 발견 5·9는 데이터 오설정 시의 위험이므로 실 데이터 점검이 함께 필요하다.
  - 인벤토리 델리게이트 구독자(`Source/WxGame/MVVM/WxViewModel_*`)의 수명·해제 정확성은 WxGame 리뷰 범위로 넘겼다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 22파일 — `/module-review`로 갱신*
