# WxInventory — 코드 리뷰

> Lyra 인벤토리 패턴(FastArray + 복제 서브오브젝트)을 충실히 따르는 잘 정돈된 서버 권위 모듈이다. 데이터 모델(Definition/Fragment/Instance)·권위 API·통지 델리게이트의 경계가 명확하고 CLAUDE.md 규칙(Copyright·Wx prefix·BlueprintCallable 위치·Handle prefix·WxCore 외 참조 없음·람다 미사용) 위반은 없다. 19개 소스 전부를 읽었으며 매니저의 FastArray 복제·통지 경로와 장비/인스턴스 수명주기를 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

가장 먼저 손댈 것: 클라이언트의 슬롯 제거 통지(`PreReplicatedRemove`)에서 정의 합계 `NewCount`가 제거분만큼 과다 계산되어 서버와 값이 어긋나고, 그 잘못된 총량이 클라 UI에 지속된다.

## 결과

### 1. 🔴 클라이언트 슬롯 제거 시 정의 합계(NewCount)가 제거분만큼 과다 계산됨
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:59` (PreReplicatedRemove) → `:600` (NotifyStackChangedFromList)
- **범주**: 버그/정확성
- **문제**: `NotifyStackChangedFromList`는 `NewCount = GetTotalItemCountByDefinition(ItemDef)`로 합계를 **재계산**한다. FastArray의 `PreReplicatedRemove`는 엔트리가 배열에서 실제로 제거되기 **전**에 호출되므로("Pre"), 이 시점의 `GetTotalItemCountByDefinition`은 제거 대상 엔트리의 StackCount를 아직 합산에 포함한다. 따라서 클라이언트가 방송하는 `NewCount`는 실제보다 제거분(`Entry.LastObservedCount`)만큼 크다. 서버 경로(`RemoveItemInstance`/`ConsumeItemsByDefinition`)는 엔트리를 먼저 제거한 뒤 재계산해 정확한 값을 내보내는데, 클라이언트만 어긋난다. 델리게이트 설계 계약("서버 변경·클라 OnRep 양 경로가 동일 진입점으로 수렴해 동일 값 발행")이 제거 경로에서 깨진다. 게다가 다른 엔트리가 바뀌기 전까지 재계산이 다시 일어나지 않아 클라의 잘못된 총량이 지속된다(예: 한 슬롯을 전량 소진하면 클라 UI 총량이 소진 전 값으로 남음). Delta는 정확하므로 Delta만 소비하는 구독자는 영향 없다.
- **제안**: 제거 경로에서 재계산에 의존하지 말고 제거분을 반영한 값을 넘긴다. 예: `PreReplicatedRemove`에서 `GetTotalItemCountByDefinition(Def) - Entry.LastObservedCount`를 산출해 통지하거나, 스택 통지 진입점이 명시적 `NewCount`를 받도록 시그니처를 바꾼다(이미 `NotifySlotChangedFromList`는 명시 값을 받는다).
- **확신도**: 높음(FastArray 콜백 순서상 기전 확실). 크래시·서버 데이터 손상은 없고 클라 표시 값에 국한된다.

### 2. 🟡 FastArray 슬롯 추가 통지가 서브오브젝트 미해석 시 유실될 수 있음
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:82`
- **범주**: 버그/정확성 (복제 순서)
- **문제**: `PostReplicatedAdd`는 `Entry.Instance`가 유효할 때만 슬롯/합계 통지를 발행한다(`if (Entry.Instance)`). 그러나 FastArray 엔트리와 복제 서브오브젝트(`UWxItemInstance`)는 별개 복제 경로라, 엔트리 참조가 언맵 NetGUID 상태로 먼저 도착하면 이 시점에 `Instance == nullptr`이 될 수 있다. 그러면 슬롯 추가 통지가 한 번도 발행되지 않는다. 더구나 같은 콜백에서 `Entry.LastObservedCount = Entry.StackCount`로 갱신하므로, 이후 참조가 해석되어도 `PostReplicatedChange`의 Delta(`StackCount - LastObservedCount`)가 0이 되어 "추가"를 복구할 기회조차 없다. 결과적으로 클라 UI가 특정 아이템 획득을 놓칠 수 있다.
- **제안**: `Instance`가 null이면 `LastObservedCount`를 갱신하지 말고(다음 `PostReplicatedChange`가 add 델타로 흘리도록) 두거나, 인스턴스 측 `ItemDef` OnRep 해석 시 소속 슬롯 통지를 재유도하는 보정 경로를 둔다.
- **확신도**: 중간. 대부분 동일 번치에서 서브오브젝트가 먼저 해석되어 정상 동작하지만, 언맵 참조 상황에서 드러나는 잠재 결함이다.

### 3. 🟡 머지 루프 내 브로드캐스트가 Entries 참조·인덱스를 든 채 재진입 시 무효화 위험
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:287`
- **범주**: 버그/정확성 (재진입)
- **문제**: `AddItemDefinition` 머지 루프는 `const TArray<FWxInventoryEntry>& Entries = InventoryList.GetEntries();`로 내부 배열 참조를 잡아 두고, 루프 안에서 `NotifySlotChangedFromList`/`NotifyStackChangedFromList`로 외부 구독자에 방송한다. 어떤 구독자가 이 콜백에서 동기적으로 인벤토리를 변경하면(`ConsumeItemsByDefinition`/`AddItemDefinition` → 엔트리 제거·추가로 배열 shift/realloc) 잡아 둔 `Entries` 참조·인덱스가 무효화되어 잘못된 엔트리 처리 또는 realloc 시 해제 후 접근이 발생할 수 있다. `ConsumeItemsByDefinition`은 배열 변경이 끝난 뒤 `Changes`를 순회하며 방송하므로 상대적으로 안전하다 — 머지 루프가 가장 취약하다.
- **제안**: 변경 결과를 로컬 배열에 모아 루프 밖에서 일괄 통지하거나, 최소한 인덱스가 아니라 인스턴스 포인터 기준으로 재조회한다. 현재 구독자가 UI(비변경)라 실무상 발현되지 않아 우선순위는 낮다.
- **확신도**: 낮음(의도된 설계일 수 있고 현 구독자 특성상 안전).

### 4. 🟢 Build.cs의 DeveloperSettings 미사용 의존성
- **위치**: `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs:25`
- **범주**: 중복/복잡도 (데드 의존성)
- **문제**: `DeveloperSettings`가 PrivateDependency로 선언돼 있으나 모듈에 `UDeveloperSettings` 파생·`GetDefault<U...Settings>` 사용이 없다(전수 검색 시 Build.cs·README만 매치, `GetDefault` 히트는 무관한 `GetDefaultColorForGrade`). 템플릿 잔재로 보인다.
- **제안**: 설정 클래스 도입 계획이 없다면 의존성 목록에서 제거.
- **확신도**: 높음.

### 5. 🟢 `GrantReward`가 `SourceActor->GetWorld()`를 null 검사 없이 스폰에 사용
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:39`
- **범주**: 버그/정확성 (미처리 실패 경로)
- **문제**: `UWorld* World = SourceActor->GetWorld();` 이후 null 검사 없이 `World->SpawnActorDeferred`(:76)를 호출한다. 픽업 보상이 하나라도 있고 `World`가 null이면 역참조 크래시다. 권위 액터는 통상 유효한 월드를 가지므로 실현 가능성은 낮다.
- **제안**: 픽업 스폰 루프 진입 전 `if (!World) return;`로 방어.
- **확신도**: 낮음.

### 6. 🟢 Charges + Stackable(MaxStack>1) 동시 부착 시 충전량 공유가 비일관 — 가드 없음
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:284` · `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:106`
- **범주**: 설계/구조
- **문제**: 설계상 Charges(인스턴스별 충전)와 Stackable(슬롯 머지)은 직교이며 Charges 아이템은 사실상 MaxStack=1을 전제한다. 그러나 두 Fragment를 MaxStack>1로 함께 부착하면 머지 경로가 여러 물리 아이템을 한 인스턴스(=한 충전 카운터)로 합쳐 "N개 소유인데 충전은 하나로 공유"되는 비일관 상태가 되고, 머지 분기는 `NotifyChargeChangedFromSource`도 호출하지 않아 충전 통지까지 누락한다. 런타임/에디터 가드가 없다.
- **제안**: 무거운 런타임 가드 대신 Fragment 헤더/README에 "Charges와 Stackable(MaxStack>1) 동시 부착 금지"를 명시하거나 에디터 `IsDataValid`로 경고만 준다.
- **확신도**: 낮음(오설정 전제, 의도된 직교 설계의 경계 사례).

### 7. 🟢 값 반환 override가 `Super::` 미호출 — 규칙 5의 문자적 위반이나 UE 관용상 의도됨
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:18` (EnterState) · `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12` (GetPrimaryAssetId) · `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp:18` (IsSupportedForNetworking)
- **범주**: 규칙 위반 (CLAUDE.md 규칙 5)
- **문제**: 세 override 모두 부모 반환을 대체하는 값 반환 함수라 `Super::`를 호출하지 않는다. StateTree Task의 `EnterState`는 자체 실행 상태를 반환하는 것이 표준 관용이고(Super 호출 시 `Running` 반환으로 오동작), `GetPrimaryAssetId`/`IsSupportedForNetworking`도 부모 반환을 의도적으로 대체한다.
- **제안**: 코드 변경 불필요. 규칙 5의 예외(값 반환 대체 override)로 명시 관리하는 편이 낫다.
- **확신도**: 낮음(관용상 정당).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` · `Public/Inventory/WxInventoryManagerComponent.h` · `Private/Items/WxItemInstance.cpp` · `Private/Inventory/WxEquipmentComponent.cpp` · `Private/WxRewardLibrary.cpp` · `Private/Items/WxItemPickup.cpp`
- **훑은 파일**: `Public/Items/WxItemFragment.h` · `Private/Items/WxItemFragment.cpp` · `Public/Items/WxItemDefinition.h` · `Private/Items/WxItemDefinition.cpp` · `Public/Items/WxRewardTableRow.h` · `Public/Inventory/WxRewardStateTreeNodes.h` · `Private/Inventory/WxRewardStateTreeNodes.cpp` · 나머지 Public 헤더 · `WxInventory.Build.cs` · `WxInventoryModule.*`
- **모듈 경계 확인**: `WxInteractionSource.h`·`WxCollisionChannels.h`는 `WxCore`에 위치함을 확인 — 「WxCore 외 Wx 플러그인 참조 금지」 위반 없음. `Build.cs` 의존성도 WxCore 외 Wx 플러그인 없음.
- **미검토 / 한계**: 클라-서버 복제 순서/엣지(리스닝 서버 vs 데디케이트, 레이트조인 시 SubObject 등록 타이밍, 언맵 GUID 해석 순서)는 정적 분석 근거이며 네트워크 재현 테스트로 검증하지 않았다(발견 1·2 관련). `.uasset`(픽업 상속 BP, ItemDefinition 데이터 자산의 실제 Fragment 조합·디폴트값)과 UI 구독자 배선은 범위 밖.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 19파일 — `/module-review`로 갱신*
