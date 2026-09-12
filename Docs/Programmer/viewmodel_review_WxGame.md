# WxGame — ViewModel 코드 리뷰

> `Source/WxGame/MVVM/`의 20개 h/cpp와 데이터 공급·HUD 수명 경로를 검토했다. 이전 리뷰의 재연결·종료 결함은 현재 코드에서 해결되었으며, 승인되어 유지된 사용 조건을 제외하고 새로 확정한 기능 결함은 없다. 일반 ViewModel의 BlueprintCallable 선언과 현재 프로젝트 규칙의 충돌 1건을 남긴다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 일반 ViewModel의 BlueprintCallable 선언이 현재 규칙과 충돌한다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:42`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:45`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:80`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.h:47`
- **범주**: 규칙 위반
- **문제**: 일반 VM에 선언된 `RequestAdvance`, `RequestInteract`, `RequestCycle`, `SetCurrentCategory`, `RequestUseConsumable`이 `BlueprintCallable`을 사용한다. 현재 `AGENTS.md`는 Blueprint Function Library와 Blueprint Async Action의 팩토리 함수에만 이 지정자를 허용한다. 다섯 함수는 그 예외에 해당하지 않는다. 컴파일 오류나 확인된 게임 동작 결함을 뜻하지는 않는다.
- **제안**: 기존 WBP 호출·카테고리 setter 바인딩을 확인한 뒤, 현재 규칙을 따르는 함수 라이브러리 진입점 등으로 호출 계약을 옮긴다. 지정자만 제거하여 기존 BP 호출을 깨뜨리지 않는다.
- **확신도**: 높음

## 이전 발견의 재검증

- Dialogue·Quest는 `Initialize` 시작 시 `Deinitialize`를 호출하여 이전 소스 구독을 먼저 해제한다. 같은 소스 반복 초기화와 다른 소스 교체에 대한 이전 지적은 해결되었다.
- InteractionList의 `Deinitialize`는 `StopObserving`을 호출하여 Ready 구독과 관찰 PC를 정리한다. `StartObserving`도 이전 연결을 먼저 정리하므로 종료 후 재연결 및 handle 덮어쓰기 지적은 해결되었다.
- Dialogue·Quest·InteractionList는 살아 있는 VM의 종료 시 빈 표시 상태를 통지하고 GC 중에는 FieldNotify를 생략한다. 각 전용 Resolver에도 `DestroyInstance`의 명시적 종료 호출이 있다.
- `.codex/worklog/2026-09-12-ViewModel-개선-범위-축소.md`의 최종 승인 범위를 적용했다. Inventory 직접 주입과 Resolver 파생 타입·타입 검증 확장은 의도적으로 되돌린 정책이므로 미해결 버그로 다시 집계하지 않는다.
- 획득 VM은 토스트 등 다른 위젯이 계속 참조할 수 있는 독립 표시 객체이다. `LastAcquiredItem` 교체만으로 이전 객체가 불필요해졌다고 단정할 수 없으므로, 교체 시 무조건 종료하라는 기존 모듈 리뷰의 제안은 이번 기능 결함 목록에 포함하지 않는다. 약한 델리게이트 구독은 기본 VM의 `BeginDestroy → Deinitialize`에서 정리된다.

## 유지되는 사용 조건

| 대상 | 현재 사용 조건 |
| --- | --- |
| Inventory | owning PlayerController의 인벤토리를 관찰한다. 보관함·상인 Actor의 컴포넌트를 직접 주입하는 범용 목록 API는 제공하지 않는다. |
| InventoryItem | 슬롯 또는 정의 합계로 실제 인벤토리 컴포넌트를 직접 받는다. 직접 초기화는 BeginPlay 이후 컴포넌트가 필요하며, PC 관찰 방식과 달리 나중 Ready에 자동 연결되지 않는다. |
| InventoryItem.RequestUseConsumable | 표시 중인 슬롯과 무관하게 인벤토리 공통 소비 명령을 실행한다. 헤더에 명시된 단일 소비 아이템 정책이다. |
| Dialogue·Quest·InteractionList Resolver | 고정 기본 VM 타입을 만든다. 임의 파생 타입 생성은 지원 계약에 포함되지 않는다. |
| PlayerCharacter·Ability Resolver | 생성 시점의 Pawn에서 공유 VM을 찾는다. 기본 HUD는 `WxPlayerLayoutComponent`의 Pawn 교체 처리로 다시 생성한다. 별도 장수 위젯은 자체 재연결 정책이 필요하다. |
| BossDisplay | 월드의 교전 보스 목록을 관찰하고 현재 선택을 유지한다. 특정 보스를 외부에서 직접 주입하는 범용 캐릭터 VM이 아니다. EndPlay 시 보스의 교전 종료 신호가 표시 대상을 갱신한다. |

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/MVVM/` 아래 `WxViewModel_BossDisplay`, `WxViewModel_Dialogue`, `WxViewModel_InteractionList`, `WxViewModel_Inventory`, `WxViewModel_InventoryItem`, `WxViewModel_Quest`, `WxViewModel_QuestObjective`, `WxViewModelResolver_Ability`, `WxViewModelResolver_BossCharacter`, `WxViewModelResolver_PlayerCharacter`의 h/cpp 20개 전체.
- **교차 확인**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`.
- **훑은 파일**: `Source/WxGame/Character/WxEnemyCharacter.cpp`의 초기화·교전·EndPlay, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`의 Ready/Ended·조회·이벤트·소비 명령 경로. 이전 ViewModel 리뷰·모듈 리뷰 및 최종 범위 축소 작업 기록도 확인했다.
- **미검토 / 한계**: BP/WBP 바인딩·이벤트 그래프, 실제 UI 표시, PIE·네트워크 실행은 검증하지 않았다. WxUI 자체 ViewModel 전체는 별도 리뷰 대상이다. 모듈 전체 65개 소스를 통독한 결과는 아니다. 소스 변경이 없는 정적 리뷰이므로 빌드·자동화 테스트는 이번에 실행하지 않았다. 이전 작업 기록의 빌드·테스트 성공은 이번 검증으로 계산하지 않는다. `Docs/Programmer/module_review_WxGame.md`는 보존했다.

---
*문서 기준 커밋 `dfdad10a2` · 리뷰일 2026-09-12 · 소스 65파일 — `/module-review`로 갱신*
