# WxGame — ViewModel 독립성·재사용성 코드 리뷰

> **2026-09-12 최종 적용 범위**: 아래는 수정 전 리뷰 기록이다. Dialogue·Quest의 재연결 전 구독 해제, InteractionList의 준비 대기 취소, 세 VM의 종료 시 빈 상태 통지와 개별 Resolver의 종료 호출만 유지했다. 사용자가 범위 축소를 승인하여 Inventory 직접 주입과 Resolver 파생 타입·타입 검증 확장은 되돌렸다. 해당 재사용 제약은 현재 사용처에 맞춰 유지한다.
> 기본 보스 선택 정책, 아이템 공통 소비 명령과 기존 HUD의 Pawn 교체 정책은 유지한다.

> 표시용 데이터와 게임 브리지의 모듈 분리는 적절하지만, WxGame의 ViewModel은 대부분 실제 도메인 컴포넌트를 요구한다. 같은 게임에서 위젯을 바꾸어 쓰는 재사용성은 확보되어 있고, 데이터 소스 교체·위젯 풀 재사용·다른 게임으로의 이식에는 아래 제약이 있다.
> 이번 검토는 `Source/WxGame/MVVM/`의 20개 h/cpp 전체와 관련 소유자·기본 ViewModel·HUD 수명 경로에 한정한다. 모듈 전체 소스 67파일을 통독한 리뷰는 아니다. 기존 `module_review_WxGame.md`는 보존한다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 Dialogue·Quest의 재초기화가 이전 소스 구독을 남긴다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:15`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp:17`
- **범주**: 버그/정확성
- **문제**: 두 `Initialize`는 기존 연결을 해제하지 않고 캐시를 새 소스로 덮은 뒤 `AddDynamic`을 호출한다. `Initialize(A)` 후 `Initialize(B)`하면 A의 구독은 남고, 이후 `Deinitialize`는 B만 해제한다. Dialogue에서는 A의 대사 이벤트가 B의 화면을 덮지만 `RequestAdvance`는 B로 전송되어 표시와 명령 대상이 달라진다. Quest에서는 A의 변경에도 B를 불필요하게 다시 조회한다. 동일 소스의 반복 초기화에도 중복 등록 시도를 한다. 현재 Resolver는 매번 새 객체를 만들므로 기본 생성 경로에서 항상 발생하는 버그로 단정하지 않는다.
- **제안**: 기존 소스와 같은 경우 처리 규칙을 정하고, 교체 전 기존 구독을 해제한다. null 입력을 연결 해제로 사용할지도 명시한다. `Initialize(A) → Initialize(B) → A/B 이벤트 → Deinitialize` 시나리오로 검증한다.
- **확신도**: 높음

### 2. 🟡 InteractionList는 Deinitialize 이후에도 대기하던 스캐너에 다시 연결될 수 있다

- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:47`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:98`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:9`
- **범주**: 버그/정확성
- **문제**: 스캐너가 없는 PC에 `StartObserving`하면 정적 Ready 이벤트를 구독한다. 이후 `Deinitialize`는 `StopObserving`이나 `ObservedController.Reset`을 하지 않는다. PC의 스캐너가 뒤늦게 준비되면 `HandleScannerReady`가 다시 `Initialize`를 호출하여 종료한 VM을 재활성화한다. 또한 대기 중 `StartObserving`을 반복하면 저장한 handle을 덮어써 앞선 구독을 명시적으로 제거할 수 없다. 정적 구독 정리는 연결 성공 또는 `BeginDestroy`에만 있어, 살아 있는 객체를 풀에 돌려놓거나 다른 대상으로 전환하는 수명 계약이 불완전하다.
- **제안**: 외부 호출용 종료 경로에서 대기와 현재 연결을 모두 정리한다. `StartObserving`은 기존 대기를 먼저 끝내고, 내부 연결 교체와 전체 관찰 종료를 분리한다. 종료 후 Ready 이벤트가 와도 목록과 연결이 바뀌지 않는지 확인한다.
- **확신도**: 높음

### 3. 🟡 Dialogue·Quest·InteractionList의 종료가 화면의 빈 상태를 통지하지 않는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:23`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp:25`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:47`
- **범주**: 버그/정확성
- **문제**: Dialogue는 `Speaker`·`LineText`를 유지한다. Quest는 `Objectives.Reset()`만 실행하고 FieldNotify를 내보내지 않으며 `QuestTitle`·`bHasActiveQuest`를 유지한다. InteractionList도 `Entries.Reset()`의 통지가 없고 `SelectedIndex`를 유지한다. VM과 위젯이 살아 있는 상태에서 종료하면 마지막 대사/퀘스트/목록이 UI에 남을 수 있다. 기본 `UWxViewModel::Deinitialize`는 이미지 요청만 취소하므로 이 값을 대신 초기화하지 않는다. 반면 Inventory는 종료 시 가용성과 목록을 명시적으로 초기화·통지한다.
- **제안**: 살아 있는 객체의 종료에는 각 공개 필드를 빈 값으로 바꾸고 관련 FieldNotify·파생 필드 통지를 발행한다. GC 중에는 통지하지 않는 경로를 유지한다. 표시를 일부러 유지하려면 연결 해제와 전체 Reset을 별도 계약으로 제공한다.
- **확신도**: 높음

### 4. 🟡 Inventory 목록은 PlayerController 소유 인벤토리만 받을 수 있다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.h:31`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:97`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:18`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:199`
- **범주**: 설계/구조
- **문제**: 공개 데이터 연결 API가 `StartObserving(APlayerController*)` 하나이고 직접 컴포넌트를 받는 `BindSource`는 private이다. 검색과 Ready 필터 모두 PC 소유를 전제한다. 같은 `UWxInventoryComponent`를 보관함·상인·루팅 대상 Actor에 붙여도 그 목록을 이 VM에 직접 주입할 수 없다. 하위 `InventoryItem`에는 컴포넌트 직접 주입 경로가 있어 목록과 항목의 재사용 범위도 다르다. 현재 플레이어 HUD 용도에는 타당한 정책이나 범용 인벤토리 목록으로 쓰려면 수정이 필요하다.
- **제안**: `Initialize(UWxInventoryComponent*)` 등 직접 연결 계약을 공개하고, PC 관찰은 편의 어댑터로 유지한다. 우선 임의 Actor 소유의 실제 컴포넌트를 받을 수 있게 하면 되며, 테스트 더블이나 다른 도메인 구현이 실제로 필요해질 때 데이터 공급 인터페이스를 추가한다.
- **확신도**: 높음

### 5. 🟡 일부 Resolver는 요청한 파생 ViewModel 타입을 생성하지 않는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:67`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp:75`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:160`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp:22`
- **범주**: 설계/구조
- **문제**: 나열한 Resolver는 `ExpectedType`을 사용하지 않고 정해진 기본 클래스를 생성하거나 조회한다. 위젯에서 추가 필드를 가진 파생 VM을 기대하도록 구성해도 반환 객체에는 그 필드가 없다. Inventory·Item·Boss Resolver는 요청 클래스의 상속 관계와 abstract 여부를 검사하고 해당 클래스로 생성하므로 확장 계약이 일관되지 않다. Ability Resolver 역시 ASC 컨테이너가 소유한 고정 타입을 반환하므로 임의 파생 타입 Resolver로 볼 수 없다.
- **제안**: 위젯별 VM은 Inventory와 같은 요청 타입 생성 규칙을 적용한다. 공유 VM은 타입까지 공유 키에 포함할지, 고정 타입만 지원하고 부적합한 요청을 명시적으로 거절할지 정한다. 현재 기본 타입 WBP에서의 실패가 확인된 것은 아니며 파생 재사용의 제한이다.
- **확신도**: 높음

## 클래스별 독립 사용 조건과 재활용성

여기서 독립 사용은 특정 WBP·HUD·다른 VM 없이 C++에서 생성하고 데이터를 공급할 수 있는지를 뜻한다. 다른 프로젝트에 이식할 수 있는지는 별도 축이다. `WxGame.Build.cs`가 UI와 여러 도메인을 조립하므로 이 모듈의 클래스를 다른 플러그인에서 참조하는 방식을 공용화 해법으로 쓰면 안 된다.

| 클래스 | 독립 사용 조건 / 데이터 주입 | 재활용성 판단 |
| --- | --- | --- |
| `UWxViewModel_QuestObjective` | `NewObject`와 `SetObjectiveText(FText)`만 필요하다. | 높음. 런타임 퀘스트 컴포넌트나 부모 VM을 요구하지 않는다. 다만 WxGame 소속이라 범용 배포에는 모듈 위치를 조정해야 한다. |
| `UWxViewModel_Dialogue` | `Initialize(UWxDialogueSessionComponent*)`로 실제 세션을 직접 공급한다. `HandleLineChanged`로 표시만 공급할 수 있으나 원래 이벤트 콜백이다. | 중간. 특정 PC 없이 동작한다. 세션 명령 `RequestAdvance`까지 포함하므로 순수 표시 데이터 객체는 아니다. 소스 교체·Reset 보완이 필요하다. |
| `UWxViewModel_Quest` | `Initialize(UWxQuestComponent*)`로 실제 저널을 공급한다. VM 자체는 GameState를 요구하지 않는다. | 중간. GameState 제한은 Resolver의 정책이다. 텍스트/목록 직접 주입 API는 없으며 소스 교체·Reset 보완이 필요하다. |
| `UWxViewModel_InteractionList` | `Initialize(UWxInteractionScannerComponent*)`로 직접 주입하거나 PC를 관찰한다. | 중간. 다른 스캐너로 재연결은 기존 컴포넌트 구독을 해제한다. 관찰 종료·Reset은 불완전하다. `RequestInteract`·`RequestCycle`은 스캐너 명령이다. |
| `UWxViewModel_Inventory` | PC를 공급하고 그 PC에 플레이가 시작된 `UWxInventoryComponent`가 있어야 한다. | 플레이어 UI 안에서는 높음. 위젯별 필터 상태와 자식 VM 재사용, Ready/Ended 관찰·종료 통지가 있다. 다른 소유자의 인벤토리 목록에는 낮음. |
| `UWxViewModel_InventoryItem` | `Initialize(Inventory, Instance/Definition)`로 직접 공급하거나 `StartObserving(PC, Definition)`을 쓴다. 인벤토리가 없어도 Definition 정적 표시가 가능하다. | 중상. 슬롯과 정의 합계 두 방식 및 재초기화를 지원한다. 직접 공급 방식은 이미 BeginPlay가 끝난 컴포넌트를 요구한다. 이 경로는 `ObservedController`가 없으므로 나중 Ready 이벤트로 자동 연결되지 않는다. |
| `UWxViewModel_BossDisplay` | `StartObserving(UWorld*)`가 `AWxEnemyCharacter`, 교전 태그와 정적 교전 이벤트를 탐색한다. | 게임 내 보스 HUD에는 적합하다. 월드/보스 구현/표시 대상 선택 정책이 묶여 특정 적을 외부에서 직접 주입하는 범용 캐릭터 VM으로는 부적합하다. |

`InventoryItem.RequestUseConsumable`은 바인딩된 슬롯을 사용하는 명령이 아니다. 헤더 `Source/WxGame/MVVM/WxViewModel_InventoryItem.h:43`과 구현 `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:160`에 따라 인벤토리의 공통 소비 명령을 전달한다. 현재 단일 소비 아이템 정책은 명시된 의도이므로 버그로 세지 않았지만, 모든 아이템 셀에 붙일 범용 사용 버튼으로 재활용하면 안 된다.

Resolver는 위젯 생성 시점의 컨텍스트를 찾는 어댑터이다. Dialogue는 owning PC의 세션, Quest는 월드 GameState의 퀘스트 컴포넌트를 요구한다. PlayerCharacter는 `AWxPlayerCharacter`를 요구하고 Ability는 Pawn의 `IAbilitySystemInterface`를 요구한다. PlayerCharacter/Ability Resolver 자체에는 Pawn 교체 관찰이 없지만, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp:43`의 HUD 교체 경로가 이를 보완하므로 기본 HUD의 재빙의 버그로 보고하지 않았다. 이 HUD 밖에서 장수하는 위젯은 별도 재연결 정책이 필요하다.

개선 순서는 재초기화·종료 계약을 먼저 통일하고, 다음으로 실제 필요한 보관함/상인 등의 데이터 소스 주입을 여는 것이 적절하다. 모든 ViewModel에 새 인터페이스나 프레임워크를 도입할 근거는 현재 코드만으로 충분하지 않다.

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/MVVM/` 아래 20개 h/cpp 전부. `WxViewModel_BossDisplay`, `WxViewModel_Dialogue`, `WxViewModel_InteractionList`, `WxViewModel_Inventory`, `WxViewModel_InventoryItem`, `WxViewModel_Quest`, `WxViewModel_QuestObjective`, `WxViewModelResolver_Ability`, `WxViewModelResolver_BossCharacter`, `WxViewModelResolver_PlayerCharacter`의 헤더와 구현이다.
- **교차 확인**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/README.md`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`.
- **훑은 파일**: `Source/WxGame/Character/WxEnemyCharacter.cpp`의 교전·EndPlay 경로, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`의 Ready·EndPlay 경로, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`와 대응 cpp의 소비 명령 위치.
- **미검토 / 한계**: BP/WBP의 실제 바인딩과 이벤트 그래프, PIE·네트워크 실행, Unreal MVVM 내부 타입 검증은 실행하지 않았다. WxUI 자체 ViewModel 전체는 별도 리뷰 대상이다. 독립 사용·재연결 시나리오는 C++ 흐름으로 확인했으며 실제 기본 HUD에서 모두 발현했다는 뜻은 아니다. 소스 변경이 없어 빌드는 실행하지 않았다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 67파일 — `/module-review`로 갱신*
