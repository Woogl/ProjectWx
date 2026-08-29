# WxUI — 코드 리뷰

> CommonUI 레이어·팝업·HUD와 ASC 기반 MVVM을 연결하는 모듈로, 구독 해제와 위젯 풀 재사용을 다루는 방어 코드가 전반적으로 갖춰져 있다. 이번 리뷰는 README, 플러그인/Build 설정, Public·Private 63개 C++ 소스 중 서브시스템·팝업·MVVM·인디케이터·탭 위젯의 수명주기와 최근 변경된 Ability VM 경로를 깊게 검토하고 나머지 소스를 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 팝업이 버튼 외 경로로 닫히면 결과 콜백이 누락된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:86`
- **범주**: 버그/정확성
- **문제**: 결과 콜백은 버튼 클릭 또는 `KillPopup()`에서만 `HandleResultChosen`으로 전달된다. 그러나 레이어 전체를 비우는 `UWxUILibrary::DeactivateWidgetsInLayer`는 `ClearWidgets()`만 호출하고, 컨트롤러 교체는 레이아웃을 `RemoveFromParent()`한다. CommonUI back 입력이나 WBP의 직접 비활성화도 같은 문제를 만든다. `UWxConfirmationPopup`은 `NativeOnDeactivated`를 재정의하지 않아 이 경로에서 콜백이 실행되지 않고 호출자는 팝업이 닫혔다는 결과를 받지 못한다.
- **제안**: `NativeOnDeactivated`에서 아직 바인딩된 결과 콜백이 있으면 `HandleResultChosen(EWxPopupResult::Killed)`를 호출한다. 정상 버튼 경로는 먼저 언바인딩하므로 중복 실행되지 않는다.
- **확신도**: 높음

### 2. 🟡 폰 상태 태그 관찰이 현재 상태를 시드하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:263`
- **범주**: 버그/정확성
- **문제**: `WatchPawnTags`는 `Ability.Death`와 `State.Dialogue` 변경 이벤트만 등록하고 이미 보유한 태그를 즉시 확인하지 않는다. 컨트롤러 교체 뒤 이미 사망 또는 대화 상태인 폰을 관찰하면 태그 변화가 다시 일어나기 전까지 사망 화면이나 대화 화면이 열리지 않는다. 함수 진입 시 기존 대화 화면은 닫으므로 대화 중 폰 교체에서는 누락이 확정된다.
- **제안**: 이벤트 등록 직후 `HasMatchingGameplayTag`으로 두 태그를 읽고 기존 `HandleDeathTagChanged`·`HandleDialogueTagChanged` 판정을 호출해 초기 상태를 반영한다.
- **확신도**: 높음

### 3. 🟡 위젯·뷰모델 멤버 함수에 `BlueprintCallable`을 사용한다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:42`
- **범주**: 규칙 위반
- **문제**: `AGENTS.md` 규칙 5는 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action 팩토리에만 허용한다. `UWxTabListWidgetBase`의 7개 함수, `UWxButtonBase::SetButtonText`, `UWxViewModel_Ability::TryActivateAbility`는 모두 그 대상이 아닌 인스턴스 멤버 함수이다.
- **제안**: WBP 호출이 필요한 진입점은 `UWxUILibrary` 정적 함수 등 허용된 경계로 옮기고, 순수 조회 함수는 `BlueprintPure`만 유지하거나 Blueprint 노출을 제거한다.
- **확신도**: 높음

### 4. 🟢 델리게이트 타입·콜백 이름이 프로젝트 접두사 규칙과 다르다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:90`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:51`
- **범주**: 규칙 위반
- **문제**: `FOnTabContentCreated`와 `FOnTabContentCreatedNative`는 규칙 1의 `FWx` 접두사를 사용하지 않는다. `FTickerDelegate`에 바인딩되는 `UpdateCooldownState`, `FlushActivationRefresh`, `UpdateEffectState`, `FlushOwnedTagsRefresh`는 규칙 4의 `Handle` 접두사를 사용하지 않는다.
- **제안**: 델리게이트 타입을 `FWxOnTabContentCreated` 계열로, 티커 콜백을 `Handle...` 이름으로 정리한다. 다른 용도로 직접 호출되는 함수는 `Handle...` 래퍼를 두어 호출부 영향을 제한한다.
- **확신도**: 높음

### 5. 🟢 단순 델리게이트 브리지와 선형 탐색에 불필요한 람다를 사용한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:23`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43`
- **범주**: 규칙 위반
- **문제**: 규칙 3은 람다를 꼭 필요할 때만 허용한다. 동적 델리게이트 브리지는 `CreateUFunction`으로, 탭 탐색은 단순 range-for로 같은 동작을 표현할 수 있다. 두 람다는 캡처나 지역 상태를 이용해야 할 이유가 없다.
- **제안**: 첫 경로는 `FWxPopupResultDelegate::CreateUFunction`을 사용하고, 둘째 경로는 range-for 탐색으로 바꾼다.
- **확신도**: 높음

### 6. 🟢 디자인타임 아이콘 조회가 매번 모든 Input Mapping Context를 동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29`
- **범주**: 성능/안전
- **문제**: `GetIcon`은 위젯 디자이너 갱신 때마다 Asset Registry에서 모든 `UInputMappingContext`를 찾고 각 에셋을 동기 로드한 뒤 선형 탐색한다. 에디터 전용 경로이지만 IMC 수가 늘면 WBP 디자이너 반응성이 저하된다.
- **제안**: 액션·입력 타입별 해석 결과를 캐시하거나, 프로젝트 설정에 등록된 IMC만 조회 대상으로 제한한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp` 및 대응 헤더
- **훑은 파일**: `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, 나머지 Widget·Component·Subtitle·Indicator·MVVM·System 소스와 헤더
- **미검토 / 한계**: BP/WBP 에셋의 이벤트 그래프·MVVM 바인딩·레이아웃과 PIE 실행은 범위 밖이다. `WxUI.Build.cs`의 Wx 의존은 `WxCore`뿐이며, 파일 첫 줄 저작권 누락·규칙 6의 예외 사유 없는 인라인 정의·플러그인 의존 위반은 찾지 못했다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 63파일 — `/module-review`로 갱신*
