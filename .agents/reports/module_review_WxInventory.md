# WxInventory — 코드 리뷰

> 서버 권위·FastArray 복제·통지 수렴이 일관되게 설계돼 있고 코딩 규칙(접두사·저작권·인라인 금지·플러그인 의존)은 위반이 하나도 없는, 전반적으로 건강한 모듈이다. 이번 리뷰는 24개 소스 전체를 훑고 `UWxInventoryComponent`(FastArray·복제 콜백·사용/차감/충전), `UWxItemInstance`, `AWxItemPickup`, `UWxRewardLibrary` cpp를 깊게 봤으며, 권위 모델 검증을 위해 모듈 밖의 `UWxAbility_UseItem`·`UWxAbility_Interact`·`IWxInteractable`까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 StateTree 보상·리필 태스크가 0번 플레이어 컨트롤러에 고정돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:36`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:40`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 대상을 잡는다. 이 함수는 서버에서 `World` 의 PlayerController 목록 0번을 돌려주므로, README 가 명시한 "최대 4인 멀티" 환경에서는 접속 순서상 첫 플레이어 하나만 대상이 된다. 결과적으로 체크포인트 리필은 한 명의 에스트병만 채우고, 픽업 Fragment 가 없는 보상(재화 등)의 직접 지급도 그 한 명에게만 들어간다. 스플릿스크린 미사용(로컬 플레이어 1명) 전제와는 별개 문제다 — 원격 클라이언트들의 PC 도 서버 목록에 함께 들어 있기 때문이다.
- **제안**: 리필은 `World->GetPlayerControllerIterator()` 로 전 플레이어를 순회하고, 보상 직접 지급 대상은 태스크 인스턴스 데이터의 바인딩 가능한 액터 파라미터(기믹을 발동시킨 상호작용자 등)로 받는다. 단, 같은 `GetPlayerController(..., 0)` 패턴이 `WxDialogue`·`WxQuest` 에도 있어(`WxStateTreeTask_PlayDialogue.cpp:31`, `WxStateTreeTask_WaitMoveToTarget.cpp:41`) 이 모듈만 고치면 일관성이 깨진다. 프로젝트 차원에서 "협동 멀티를 실제로 지원할 것인가"를 먼저 정하고 일괄 처리할 사안이다.
- **확신도**: 중간 (프로젝트 전반의 공통 패턴이라 의도된 스코프 축소일 수 있음)

### 2. 🟡 `UseItemByDef` 가 GE 적용 실패·Effect 미설정에도 차감을 확정한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:527-565`
- **범주**: 버그/정확성
- **문제**: 두 갈래의 실패 경로가 열려 있다. (1) `Usable->Effect` 가 비어 있으면 `TargetASC`/`Spec` 이 null 인 채로 검증 블록(527-545)을 통째로 건너뛰고, 547-558 의 충전 1 감소 또는 스택 1 차감만 수행한 뒤 `true` 를 반환한다 — 아무 효과 없이 에스트병만 줄어든다. (2) `ApplyGameplayEffectSpecToSelf`(562)의 반환 핸들을 확인하지 않는다. 대상이 해당 GE 에 면역(`GrantedApplicationImmunityTags`)이거나 `ApplicationTagRequirements` 를 만족하지 않아 적용이 거부돼도 차감은 이미 끝난 뒤라 되돌릴 수 없다. 헤더 주석(`WxInventoryComponent.h:216`)은 "가용성·GE Spec 검증을 모두 통과한 뒤에만 차감한다"고 계약하지만 실제로는 Spec 생성까지만 검증한다.
- **제안**: (1)은 `Usable->Effect` 가 비었으면 `false` 로 조기 반환(사용 불가 아이템으로 취급)하고, `CanUseItemByDef` 도 같은 조건을 반영한다. (2)는 `ApplyGameplayEffectSpecToSelf` 를 먼저 호출해 반환 핸들이 유효할 때만 차감하도록 순서를 뒤집는다. 다만 순서를 뒤집으면 GE 측이 `SourceObject` 로 실린 인스턴스의 충전량을 읽을 때 차감 전 값을 보게 되므로, 현재 어떤 ExecCalc 도 그 값에 의존하지 않는지 확인이 필요하다.
- **확신도**: 중간 (면역 태그를 쓰는 GE 가 아직 없다면 (2)는 잠재 위험에 머무름)

### 3. 🟡 스택 불가 아이템은 수량만큼 인스턴스가 무제한 생성된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:312-330`
- **범주**: 성능/안전
- **문제**: `Stackable` Fragment 가 없으면 `MaxStack` 이 1 이 되어 `while (Remaining > 0)` 루프가 `StackCount` 회 반복하며 매 회 `UWxItemInstance` 를 `NewObject` 하고 FastArray 엔트리를 추가한다. 수량 상한이 어디에도 없다 — `FWxItemRewardEntry::Quantity` 는 `ClampMin = 1` 만 걸려 있고(`WxRewardTableRow.h:222`), `AWxItemPickup::Quantity` 도 하한만 보정한다(`WxItemPickup.cpp:124`). 보상 테이블에 `Stackable` 을 빠뜨린 재화 아이템 하나를 수량 10,000 으로 기입하면 그 자리에서 1만 개의 복제 서브오브젝트가 생성되어 게임 스레드가 멈추고 네트워크 대역이 포화된다. 게다가 루프 안에서 매 회 호출하는 `NotifyStackChangedFromList` 는 `GetTotalItemCountByDefinition` 로 전 엔트리를 재순회하므로 전체 비용이 O(n²) 다.
- **제안**: `AddItemDefinition` 진입부에 신규 생성 엔트리 수 상한을 두고 초과 시 경고 로그 후 절단하거나, `Stackable` Fragment 부재 + `StackCount > 1` 조합을 명시적 에러로 거부한다. 최소한 `Quantity` 에 `ClampMax` 를 걸어 저작 단계에서 막는다.
- **확신도**: 중간 (현재 데이터에서 실제로 밟히는지는 DataTable 확인 필요 — 리뷰 범위 밖)

### 4. 🟡 "인벤토리를 어떤 액터에서 찾는가" 규칙이 7곳에 복제돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:154-156`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:287-289`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp:52-54`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp:122-124`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp:85-86`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp:95-96`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:36-37`
- **범주**: 중복/복잡도
- **문제**: `Actor → Cast<APawn> → GetController() → Cast<APlayerController> → FindComponentByClass<UWxInventoryComponent>()` 라는 해석 체인이 그대로 7번 반복된다. 이는 스타일 문제가 아니라 "인벤토리는 PlayerController 가 소유한다"는 모듈의 핵심 전제가 7개 파일에 하드코딩돼 있다는 뜻이다 — 소유자가 바뀌면(예: PlayerState) 전부 찾아 고쳐야 하고, 이미 두 변종(`GetTypedOuter<APlayerController>` 경로 2곳 / Pawn 경유 경로 4곳)이 갈라져 있어 폰이 없는 Interactor 같은 엣지 케이스 처리가 사이트마다 다르다.
- **제안**: `UWxInventoryComponent` 에 `static UWxInventoryComponent* FindForActor(const AActor* Actor)` 정적 리졸버 하나를 두고 전부 그리로 모은다. (호출부 인라인 선호는 "같은 판정 한 줄"에 대한 것이고, 여기는 판정이 아니라 소유 규칙의 단일 정의점 문제라 헬퍼가 정당하다고 본다.)
- **확신도**: 중간 (중복 자체는 사실, 헬퍼 도입 여부는 취향 경계)

### 5. 🟢 `GrantReward` 가 `World` 널 검사 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:291`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:323`
- **범주**: 성능/안전
- **문제**: 291 에서 얻은 `UWorld* World` 를 323 의 `World->SpawnActorDeferred` 에서 검사 없이 쓴다. 같은 함수가 `SourceActor`·`RewardRow`·`Row`·`ItemDef`·`ItemActorClass`·`SpawnedPickup` 을 모두 방어적으로 검사하는 것과 대비된다. 파괴 중이거나 월드에서 떨어져 나온 액터가 `SourceActor` 로 들어오면(예: 사망 처리와 같은 틱에 지연 실행되는 드랍 경로) 널 역참조로 크래시한다.
- **제안**: 291 직후에 `if (!World) { return; }` 한 줄.
- **확신도**: 중간 (현재 호출부는 모두 살아 있는 액터라 실제로 밟기는 어려움)

### 6. 🟢 `RemoveItemInstance` 는 호출부가 없는 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:179`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:352-384`
- **범주**: 중복/복잡도
- **문제**: 헤더 주석이 스스로 "현재 호출부가 없다"고 적고 있고, 저장소 전체를 검색해도 정의부 외에 호출부가 없다. 30여 줄이 `ConsumeItemsByDefinition` 과 거의 같은 일(수량 조회 → 서브오브젝트 해제 → 엔트리 제거 → 슬롯·합계 통지)을 별도 경로로 중복 구현하고 있어, 앞으로 통지 규칙이 바뀔 때 함께 고쳐야 하는 부담만 남는다.
- **제안**: 장비 해제·버리기 같은 확정된 용처가 생길 때 다시 넣고 지금은 제거한다. 남긴다면 왜 남기는지(예정된 기획)를 주석에 적는다.
- **확신도**: 높음

### 7. 🟢 `FWxInventoryList::AddToEntryStack` 이 public 인데 인덱스를 검증하지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:166-172`
- **범주**: 성능/안전
- **문제**: `Entries[EntryIndex]` 를 범위 검사 없이 접근한다. `FWxInventoryList` 는 `WXINVENTORY_API` 로 노출된 public USTRUCT 이고 이 함수도 public 이라 외부에서 호출 가능한데, 유일한 정상 호출부(`AddItemDefinition`)만 인덱스 유효성을 보장한다. 같은 구조체의 `AddEntry` 가 `check` 세 개로 전제를 강하게 못 박는 것과 비교하면 방어 수준이 일관되지 않다.
- **제안**: `check(Entries.IsValidIndex(EntryIndex))` 한 줄을 추가하거나, `AddToEntryStack`·`ConsumeByDefinition` 처럼 컴포넌트만 쓰는 변경 API 를 private + `friend UWxInventoryComponent` 로 닫는다.
- **확신도**: 중간 (현재는 내부 전용이라 실害 없음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/AnimNotify/WxAnimNotify_UseItem.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`
- **교차 확인(모듈 밖, 권위 모델 검증용)**: `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **미검토 / 한계**:
  - 검증 결과 문제 없다고 판단해 발견으로 올리지 않은 항목들: (a) 모듈 의존 — `.Build.cs` 와 실제 include 모두 `WxCore` 외 Wx 플러그인 참조가 없다. (b) 코딩 규칙 — 접두사·저작권 첫 줄·`Super::` 호출은 전 파일 준수, 헤더 함수 본문은 템플릿 `FindFragmentByClass<T>` 2곳과 StateTree `GetInstanceDataType()` 2곳뿐이며 모두 예외 사유 주석이 달려 있다. `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 하나로 적법하다. (c) 픽업 중복 획득 — 두 플레이어가 같은 틱에 상호작용해도 `Destroy()` 가 액터를 즉시 garbage 로 표시해 뒤따르는 `FGameplayEventData::OptionalObject`(TWeakObjectPtr) 해석이 null 이 되므로 이중 지급은 성립하지 않는다. (d) `InventoryList(this)` 생성자 바인딩 — Lyra 의 `FLyraInventoryList` 와 동일한 검증된 패턴이라 아키타입 복사로 덮이지 않는다. (e) 인벤토리 노출 — 컴포넌트가 `AController::bOnlyRelevantToOwner` 아래 있어 타 플레이어에게 복제되지 않는다.
  - 깊이 보지 못한 부분: `FWxInventoryList` 의 FastArray 복제 콜백을 다중 엔트리 동시 제거·재연결(relevancy 재획득) 시나리오까지 시뮬레이션하지는 못했다. 클라이언트 `PreReplicatedRemove` 가 제거 건마다 합계를 재계산해 중간값을 발행하는 점은 확인했으나(최종값으로 수렴하고 현재 소비자인 ViewModel 이 Delta 를 획득 신호로 쓰지 않아 무해), 서버는 합계를 1회만 발행하는 비대칭이 있어 향후 Delta 기반 획득 토스트를 붙일 때 재확인이 필요하다.
  - DataTable(보상 테이블)·아이템 정의 자산·픽업 BP 의 실제 값은 리뷰 범위 밖이라, 3번·5번의 실제 발현 여부는 확인하지 못했다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 24파일 — `/module-review`로 갱신*
