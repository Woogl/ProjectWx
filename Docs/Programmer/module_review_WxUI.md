# WxUI — 코드 리뷰

> 비동기 위젯 push와 이미지 로딩의 재진입 방어는 명확하지만 팝업 종료 통지와 컨트롤러 교체 시 정지 해제에 빈 경로가 있다. README에서 진입점을 잡고 UI 매니저·팝업·비동기 push·어빌리티/이펙트 뷰모델·네임플레이트를 중심으로 현재 작업 트리를 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 확인 팝업의 실패·강제 종료가 결과 콜백을 잃는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:76`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:24`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80`
- **범주**: 버그/정확성
- **문제**: `ShowConfirmation`은 BeforePush만 바인딩한다. 클래스 미지정·로드 실패·레이아웃 부재·레이어 push 실패 시 `Finish(nullptr)`가 호출되지만 결과 콜백에는 도달하지 않는다. 표시된 팝업도 버튼 선택을 거치지 않고 비활성화되면 결과가 없다. `Killed`를 전달하는 `KillPopup`은 모듈 내 종료 경로에서 호출되지 않으며 비활성화 override도 없다. 결과를 기다리는 소비자는 실패나 취소를 구분할 수 없다.
- **제안**: push 완료 실패를 `Killed`로 전달하고, 팝업 비활성화 시 미처리 콜백을 한 번 완료한다. 정상 버튼 처리와 공유하는 단일 완료 게이트를 두어 중복 호출을 방지한다.
- **확신도**: 높음

### 2. 🟡 컨트롤러 교체 시 기존 UI가 요청한 정지를 해제하지 않는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:204`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:141`
- **범주**: 설계/구조
- **문제**: `HandlePlayerControllerSet`은 기존 PC 추적을 먼저 비우고 대화 화면·레이아웃을 제거한다. 이때 위젯 통지로 `RefreshGamePause`가 호출돼도 PC가 없어 반환한다. 새 PC를 저장한 뒤에도 정지를 재평가하지 않는다. 같은 월드에서 기존 PC가 살아 있는 채 교체되고 새 UI를 push하지 않는 경우 기존 정지를 풀 요청이 없다.
- **제안**: 기존 레이아웃의 정지 의사가 사라진 뒤, 기존 PC 참조를 유지한 상태에서 정지 해제를 요청하고 교체한다. `HandleCanUnpause`가 이전 위젯을 보며 해제를 거부하지 않도록 철거 순서까지 함께 정한다.
- **확신도**: 중간

### 3. 🟡 어빌리티 뷰모델을 빈 슬롯으로 재초기화하면 이전 표시가 남는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:17`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:105`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:179`
- **범주**: 버그/정확성
- **문제**: `Initialize`가 먼저 `Deinitialize`로 캐시와 표시 필드를 통지 없이 비운다. 새 태그에 매칭하는 어빌리티가 없으면 `MatchedAbility`와 `CachedAbility`가 모두 null이므로 즉시 반환한다. 기존 VM에 바인딩한 화면에는 이름·아이콘·쿨다운을 비웠다는 통지가 전달되지 않는다. 이번 후보 선택 변경에도 이 경로는 남아 있다.
- **제안**: 파괴 정리와 재초기화에 필요한 표시 초기화를 구분하고, 재초기화 시 실제 변경된 필드를 통지한다. 기존 어빌리티가 있는 VM을 매칭 없는 태그로 재초기화하는 조건을 검증한다.
- **확신도**: 중간

### 4. 🟡 네임플레이트만 생성해도 사용하지 않는 이펙트 뷰모델과 티커가 붙는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:108`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:32`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:38`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:64`
- **범주**: 성능/안전
- **문제**: Character VM 초기화가 ASC 컴포지트를 즉시 생성한다. 컴포지트는 ASC 이벤트 4개를 구독하고 아이콘이 있는 활성 GE의 VM을 모두 만든다. 유한 지속 이펙트 VM마다 코어 티커가 매 프레임 동작하므로 네임플레이트가 버프 잔여 시간을 바인딩하지 않아도 비용이 발생한다. 위젯이 구성된 다수 캐릭터에서 누적되는 구조이다.
- **제안**: 실제 표시 구독에 맞춰 이펙트 VM과 잔여 시간 갱신을 시작하거나 ASC 단위로 티커를 묶는다. 우선 동시 네임플레이트 수와 아이콘 GE 수를 늘려 비용을 측정한다.
- **확신도**: 중간

### 5. 🟢 어빌리티 VM의 BlueprintCallable이 현재 허용 범위 밖이다

- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:41`
- **범주**: 규칙 위반
- **문제**: `TryActivateAbility`는 일반 뷰모델 메서드인데 `BlueprintCallable`이다. 현재 `AGENTS.md` 규칙 5는 Function Library와 Async Action 팩토리에만 허용한다. 기존 리뷰가 주장한 VM Command 예외는 현재 제공된 규칙에는 없다.
- **제안**: BP 진입점을 Function Library로 옮기거나 실제 승인된 예외가 있다면 권위 있는 규칙에 명시한다.
- **확신도**: 높음

### 6. 🟢 티커 콜백 5개에 Handle 접두사가 없다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:39`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:409`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:221`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:234`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:65`
- **범주**: 규칙 위반
- **문제**: `UpdateCooldownState`, `FlushActivationRefresh`, `FlushOwnedTagsRefresh`, `FlushAbilityRebind`, `UpdateEffectState`가 델리게이트에 직접 바인딩된다. `AGENTS.md` 규칙 4의 Callback 명명 규칙과 어긋난다.
- **제안**: 선언·정의·바인딩을 함께 `Handle` 접두사로 통일한다.
- **확신도**: 높음

### 7. 🟢 네임플레이트의 MVVM 주입 실패가 진단되지 않는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:110`
- **범주**: 버그/정확성
- **문제**: `SetViewModelByClass`의 반환값을 무시한다. 위젯의 VM 소스 설정이 맞지 않으면 주입이 실패해도 표시만 비어 원인을 찾기 어렵다. 동일 모듈의 `WxIndicator.cpp:97`은 같은 호출의 실패를 경고한다.
- **제안**: 반환값을 확인하고 위젯 클래스와 VM 소스 설정을 확인할 수 있는 경고를 남긴다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`.
- **훑은 파일**: `Plugins/WxUI/README.md`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`의 바인딩·메뉴 가시성 부분. Public/Private 전체에서 콜백·BlueprintCallable·팝업 종료 경로를 검색했다.
- **기존 발견 재평가**: 팝업·정지 해제·VM 재초기화·선행 생성 비용·콜백 명명·주입 실패를 현재 코드로 재확인했다. 메뉴와 네임플레이트의 정책 차이는 표시 의도가 확인되지 않아 결함 목록에서 제외했다. 이펙트 종료 시 티커 핸들 미초기화와 팝업 팩토리 중복은 현재 코드에도 있으나 실질 영향이 작아 별도 발견으로 유지하지 않았다. 자막은 `Tick`에서도 `StartRow.DataTable`을 읽으므로 이전 문서의 “EnterState와 ShowRow에서만 읽는다”는 근거가 잘못되어 매 틱 바인딩 복사 제거 제안을 제외했다. 전체 VM 종료 정책의 일괄 통일보다는 입증 가능한 어빌리티 재초기화 경로로 범위를 좁혔다.
- **미검토 / 한계**: 현재 작업 트리 기준이다. WBP 계층·MVVM 바인딩·에셋 데이터, 인디케이터 투영 수식 전체, 나머지 위젯/VM 구현은 전수 검토하지 않았다. 빌드·실행·성능 측정은 하지 않았다. 컨트롤러 교체 정지 문제는 기존 PC가 같은 월드에서 유지되는 조건으로 한정하며 기존 문서의 엔진 내부 정리 세부 주장은 재검증 없이 계승하지 않았다. 의존 설정에서 다른 Wx 도메인 플러그인 참조는 발견하지 않았다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 56파일 — `/module-review`로 갱신*
