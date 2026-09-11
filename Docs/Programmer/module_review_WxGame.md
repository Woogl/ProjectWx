# WxGame — 코드 리뷰

> 게임 조립 경계는 유지되며, 이번 검토에서 즉시 치명적인 결함은 확인하지 못했다. 변경된 적 캐릭터·어빌리티 리졸버·미커밋 테스트와 기존 MVVM 수명·MetaHuman 정리 경로를 중심으로 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 획득 뷰모델 교체 시 이전 인스턴스의 구독이 남는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:114`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:117`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:56`
- **범주**: 성능/안전
- **문제**: 획득마다 새 아이템 VM을 초기화하고 `LastAcquiredItem`을 교체하지만 기존 VM은 해제하지 않는다. 기존 VM의 인벤토리 구독 네 종류가 GC까지 유지되므로 연속 획득 시 사용하지 않는 VM도 변경 이벤트를 처리한다. `UnbindSource`에서는 같은 필드를 명시적으로 `Deinitialize`한다. 영구 누수로 단정할 문제는 아니지만 이벤트 처리량이 GC 사이에 누적된다.
- **제안**: 교체 전에 기존 `LastAcquiredItem`을 `Deinitialize`한다. 이전 획득 알림이 VM을 계속 표시하는 UI라면 표시용 스냅샷과 실시간 구독의 수명을 분리한다.
- **확신도**: 높음

### 2. 🟡 위젯별 뷰모델 세 종류의 해제가 GC까지 지연된다

- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:83`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:51`, `Source/WxGame/MVVM/WxViewModel_Quest.h:60`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:47`
- **범주**: 설계/구조
- **문제**: 세 리졸버는 매번 새 VM을 만들지만 `DestroyInstance`를 구현하지 않아 뷰 해제 뒤에도 소스 구독이 GC까지 남는다. 상호작용 목록은 목록 변경 시 자식 VM을 다시 할당하며 퀘스트 VM도 목표 목록을 다시 생성한다. 기본 VM의 `BeginDestroy`가 최종 정리하므로 영구 누수는 아니다. 상호작용 VM은 `Deinitialize`에서도 스캐너 준비 신호를 해제하지 않아 해제 이후 준비 신호로 다시 초기화될 여지도 있다.
- **제안**: 위젯별 생성 VM의 `DestroyInstance`에서 `Deinitialize`를 호출하고, 상호작용 VM의 준비 신호 구독까지 함께 정리한다. 공유 어빌리티 VM에는 같은 정책을 적용하지 않는다.
- **확신도**: 높음

### 3. 🟡 MetaHuman 해제 시 리더 메시의 원래 상태를 복원하지 않는다

- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:54`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디 조립에서 리더를 숨기고 `AlwaysTickPoseAndRefreshBones`로 변경하지만 해제에서는 표시를 무조건 `true`로 바꿀 뿐 틱 옵션은 복원하지 않는다. 리더가 생존한 상태에서 이 컴포넌트만 해제하면 표시 의도가 바뀌고 화면 밖 본 갱신 비용도 유지된다.
- **제안**: 등록 시 변경한 리더와 원래 표시·틱 옵션을 보관하고 해당 등록 주기의 해제 시 복원한다.
- **확신도**: 높음

### 4. 🟢 입력 콜백 다섯 개가 Handle 접두사 규칙을 따르지 않는다

- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:110`, `Source/WxGame/Character/WxPlayerCharacter.cpp:114`, `Source/WxGame/Character/WxPlayerCharacter.cpp:123`, `Source/WxGame/Character/WxPlayerCharacter.cpp:128`, `Source/WxGame/Character/WxPlayerCharacter.cpp:129`
- **범주**: 규칙 위반
- **문제**: `Move`, `Look`, `ToggleCrouch`, `AbilityInputTriggered`, `AbilityInputReleased`는 입력 델리게이트에 바인딩되는 자체 콜백이지만 AGENTS.md의 `Handle` 접두사 규칙을 따르지 않는다. 엔진 오버라이드인 `Jump`는 이 항목에서 제외한다.
- **제안**: 선언·정의·바인딩 이름을 `Handle` 접두사로 맞춘다.
- **확신도**: 높음

### 5. 🟢 처형 프롬프트가 번역 수집 대상이 아닌 문자열이다

- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:136`
- **범주**: 설계/구조
- **문제**: 사용자에게 표시되는 `Finisher`를 `FText::FromString`으로 생성한다. 문자열에 번역 수집용 namespace/key가 없어 일반적인 텍스트 수집 경로로 번역할 수 없고 문구 수정에 코드 변경이 필요하다.
- **제안**: 저작 가능한 `FText` 기본값으로 노출하거나 `NSLOCTEXT`로 정의한다.
- **확신도**: 높음

### 6. 🟢 일반 뷰모델의 BlueprintCallable이 현재 규칙과 충돌한다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:43`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:46`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.h:47`
- **범주**: 규칙 위반
- **문제**: `RequestAdvance`, `RequestInteract`, `RequestCycle`, `RequestUseConsumable`은 일반 VM의 `BlueprintCallable` 함수이다. 현재 제공된 AGENTS.md는 Blueprint Function Library와 Blueprint Async Action 팩토리에만 이 지정자를 허용한다. 이전 리뷰가 언급한 과거 Command 예외는 현재 규칙 본문에 없다. 기능 결함과는 구분해야 한다.
- **제안**: 기존 WBP 호출을 보존하면서 허용되는 라이브러리 진입점으로 옮기거나, 의도된 VM Command 예외를 현재 규칙에 명시해 코드와 정책을 일치시킨다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Tests/WxHGTestSkillRoutingTest.cpp`.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h`, `Source/WxGame/MVVM/WxViewModel_Quest.h`. MVVM 헤더의 리졸버·BlueprintCallable 선언을 검색하고, 외부 계약인 `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`의 소멸 정리를 확인했다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE·테스트 실행과 BP/WBP 데이터 검증은 하지 않았다. 소스 수는 모듈 아래 h/cpp 64개이며 전부 통독한 수가 아니다. 현재 미커밋 `Tests/WxHGTestSkillRoutingTest.cpp`를 포함한다. 기존 소환물 태그 반납 지적은 해당 로직이 `WxEnemyCharacter`에서 제거되어 이 문서에서 제외했다. `WxAbilitySlotSwitcher` 삭제로 기존 `FlushRefresh` 콜백 지적도 제거했다. 기준 이동속도의 두 줄 중복은 현재 코드에 남지만 실질적 개선 우선순위가 낮아 독립 발견에서 제외했다. 그 외 컨트롤러·아이템 사용 등 비열거 파일은 이번에 심층 검토하지 않았다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 64파일 — `/module-review`로 갱신*
