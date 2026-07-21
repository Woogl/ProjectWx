# WxInventory — 코드 리뷰

> 전반적으로 정돈된 모듈이다. 데이터 모델(Definition/Fragment/Instance)·권위(서버) API·FastArray 복제·통지 델리게이트의 경계가 명확하고, CLAUDE.md 코딩/모듈 규칙(Copyright 첫 줄, Wx prefix, BlueprintCallable 위치, Handle prefix, WxCore 외 참조 없음, 람다 미사용)은 위반이 없다. 이번 리뷰는 19개 소스 파일 전체를 읽었고, 특히 매니저의 FastArray 복제·통지 경로와 장비/인스턴스 수명주기를 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

가장 먼저 손댈 것: 클라이언트 슬롯 제거 경로(`PreReplicatedRemove`)에서 `OnInventoryStackChanged`의 `NewCount`가 제거분만큼 과다 계산되어, 서버와 값이 어긋난다(스택 소진 시 클라 UI 총량이 갱신 전 값으로 남음).

## 발견

### 🔴 클라이언트 슬롯 제거 시 정의 합계(NewCount)가 제거분만큼 과다 계산됨
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:59-71` (PreReplicatedRemove) → `:600-609` (NotifyStackChangedFromList)
- **범주**: 버그/정확성
- **문제**: `NotifyStackChangedFromList`는 `NewCount = GetTotalItemCountByDefinition(ItemDef)`로 합계를 **재계산**한다. FastArray의 `PreReplicatedRemove`는 엔트리가 배열에서 실제로 제거되기 **전**에 호출되므로(그게 "Pre"의 의미), 이 시점의 `GetTotalItemCountByDefinition`은 제거 대상 엔트리를 아직 포함한다. 결과적으로 클라이언트가 브로드캐스트하는 `NewCount`는 실제보다 제거분(`Entry.LastObservedCount`)만큼 크다. 서버 경로(`RemoveItemInstance`/`ConsumeItemsByDefinition`)는 엔트리를 먼저 제거한 뒤 재계산하므로 정확한 값을 내보내는데, 클라이언트만 어긋난다. 이 델리게이트의 설계 계약("서버 변경·클라 OnRep 양 경로가 동일 진입점으로 수렴해 동일 값을 발행")이 제거 경로에서 깨진다. 게다가 다른 엔트리가 바뀌기 전까지 재계산이 다시 일어나지 않아 클라이언트의 잘못된 총량이 지속된다(예: Currency 총량 표시가 소진 후에도 이전 값으로 남음). Delta는 정확하므로 Delta만 쓰는 소비자는 영향 없다.
- **제안**: `PreReplicatedRemove`의 스택 합계 통지에서 재계산에 의존하지 말고 제거분을 반영한 값을 넘긴다. 예: 통지 진입점에 명시적 `NewCount`를 전달하도록 시그니처를 바꾸거나(이미 `NotifySlotChangedFromList`는 명시 값을 받음), `PreReplicatedRemove`에서 `GetTotalItemCountByDefinition(Def) - Entry.LastObservedCount`를 계산해 넘긴다.
- **확신도**: 높음(FastArray 콜백 순서상 기전 확실). 다만 크래시·서버 권위 데이터 손상은 없고 클라 표시 값에 국한된다.

### 🟡 통지 브로드캐스트가 내부 Entries 참조를 든 채 루프 중간에 발생 — 재진입 시 댕글링 위험
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:287-311` (AddItemDefinition 머지 루프)
- **범주**: 버그/정확성
- **문제**: 머지 루프는 `const TArray<FWxInventoryEntry>& Entries = InventoryList.GetEntries();`로 내부 배열 참조를 잡아 두고, 루프 안에서 `NotifySlotChangedFromList`/`NotifyStackChangedFromList`로 외부 구독자에게 브로드캐스트한다. 어떤 구독자가 이 콜백 안에서 동기적으로 인벤토리를 변경하면(예: `ConsumeItemsByDefinition`/`AddItemDefinition` 호출 → 엔트리 제거·추가로 배열 shift/realloc), 잡아 둔 `Entries` 참조와 인덱스가 무효화되어 잘못된 엔트리 처리 또는 해제 후 접근(realloc 시)이 발생할 수 있다. `ConsumeItemsByDefinition`도 루프 결과(`Changes`)를 순회하며 브로드캐스트하지만 그 시점엔 배열 변경이 끝나 상대적으로 안전하다 — 머지 루프가 가장 취약하다.
- **제안**: 브로드캐스트를 루프 밖으로 모으거나(변경 결과를 로컬 배열에 수집 후 일괄 통지), 최소한 인덱스가 아니라 인스턴스 포인터 기준으로 재조회한다. 실무상 구독자가 UI(비변경)라 현재는 발현되지 않으므로 우선순위는 낮다.
- **확신도**: 낮음(의도된 설계일 수 있고, 현재 구독자 특성상 실무상 안전)

### 🟢 Build.cs의 DeveloperSettings 미사용 의존성
- **위치**: `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs:25`
- **범주**: 중복/복잡도
- **문제**: `DeveloperSettings`가 PrivateDependency로 선언돼 있으나 모듈 어디에도 `UDeveloperSettings` 파생·`GetDefault<>` 사용이 없다(전수 검색 결과 Build.cs 한 줄만 매치). 템플릿 잔재로 보이는 데드 의존성이다.
- **제안**: 설정 클래스 도입 계획이 없다면 의존성 목록에서 제거.
- **확신도**: 높음

### 🟢 Charges + Stackable(MaxStack>1) 동시 부착 시 충전량 공유가 비일관적 — 가드 없음
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:284-312` (머지 경로), `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:106-146`
- **범주**: 설계/구조
- **문제**: 설계상 Charges(인스턴스별 충전)와 Stackable(슬롯 머지)은 직교이며 Charges 아이템은 사실상 MaxStack=1을 전제한다(README: "Charges 단독 부착 시 스택 유지·충전만 소모"). 그러나 두 Fragment를 함께(그리고 MaxStack>1로) 부착하면 머지 경로가 여러 물리 아이템을 한 인스턴스(=한 충전 카운터)로 합쳐, "3개 소유"인데 충전은 하나로 공유되는 비일관 상태가 된다. 이를 막는 런타임/에디터 가드가 없다.
- **제안**: 프로젝트 성향상 무거운 런타임 가드는 지양하므로(문서·명시 규칙 선호), Fragment 헤더/README에 "Charges와 Stackable(MaxStack>1) 동시 부착 금지"를 명시하거나 에디터 `IsDataValid`로 경고만 준다.
- **확신도**: 낮음(오설정 전제, 의도된 직교 설계의 경계 사례)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, 나머지 헤더 및 `WxInventoryModule.*`
- **미검토 / 한계**: 모듈 규모가 작아 19개 소스 전부를 읽었다. 다만 클라이언트-서버 복제 순서/엣지(리스닝 서버 vs 데디케이트, 레이트조인 시 SubObject 등록 타이밍)는 정적 분석에 근거했을 뿐 실제 네트워크 재현 테스트로 검증하지는 않았다. BP/WBP 내부(픽업 상속 BP의 상호작용 컴포넌트 배선, UI 구독자)는 범위 밖.

---
*문서 기준 커밋 `702fc70f` · 리뷰일 2026-07-22 · 소스 19파일 — `/module-review`로 갱신*
