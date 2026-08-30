# WxUI — 코드 리뷰

> CommonUI 레이어 스택·팝업·HUD와 ASC 기반 MVVM을 잇는 모듈로, 구독 해제·위젯 풀 재사용·GC 수명에 대한 방어가 전반적으로 촘촘하고 의도가 주석으로 남아 있다. 이번 리뷰는 README·`WxUI.uplugin`·`WxUI.Build.cs`와 63개 C++ 소스 중 매니저 서브시스템·팝업·MVVM 뷰모델(Ability/AbilitySystem/Effect/Attribute/Indicator)·인디케이터 매니저·StateTree 노드·탭 위젯의 수명주기와 티커 경로를 깊게 보고 나머지를 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 팝업이 버튼 외 경로로 닫히면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:86`
- **범주**: 버그/정확성
- **문제**: 결과는 버튼 클릭과 `KillPopup()` 두 경로에서만 `HandleResultChosen` 으로 전달되는데, 저장소 전체를 훑어도 `KillPopup()` 을 호출하는 곳이 없다(선언·정의·`Super::` 호출뿐). 즉 실제로 결과를 내는 경로는 버튼 클릭 하나뿐이다. 반면 팝업이 닫히는 경로는 여럿이다 — `UWxUILibrary::DeactivateWidgetsInLayer` 의 `Stack->ClearWidgets()`(`Private/WxUILibrary.cpp:92`), 컨트롤러 교체 시 레이아웃 통째 파기(`Private/System/WxUIManagerSubsystem.cpp:217`), CommonUI back 입력, WBP 의 직접 `DeactivateWidget`. `UWxConfirmationPopup` 은 `NativeOnDeactivated` 를 재정의하지 않으므로 이 경로들에서는 호출자가 `EWxPopupResult::Killed` 조차 받지 못하고 대기 상태로 남는다.
- **제안**: `NativeOnDeactivated` 를 재정의해 `OnResultCallback` 이 아직 바인딩돼 있으면 `HandleResultChosen(EWxPopupResult::Killed)` 를 호출한다. 정상 버튼 경로는 콜백을 먼저 Unbind 하므로 중복 실행되지 않으며, 이렇게 하면 현재 죽어 있는 `KillPopup()` 경로도 함께 살아난다.
- **확신도**: 높음

### 2. 🟡 폰 상태 태그 관찰이 현재 태그를 시드하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:241`
- **범주**: 버그/정확성
- **문제**: `WatchPawnTags` 는 `Ability.Death`·`State.Dialogue` 의 *변경* 이벤트만 등록하고(`:263`, `:265`) 폰이 이미 들고 있는 태그를 즉시 읽지 않는다. `HandlePlayerControllerSet` 은 컨트롤러가 바뀔 때마다 이 함수를 다시 타므로(`:233` 에서 현재 폰으로 따라잡음), 이미 사망 상태이거나 대화 중인 폰을 새로 관찰하기 시작하면 다음 태그 변화가 올 때까지 사망 화면·대화 화면이 뜨지 않는다. 게다가 함수 진입부에서 `CloseDialogueScreen()` 을 무조건 부르므로(`:253`), 대화 도중 컨트롤러/폰이 교체되면 창이 닫힌 뒤 다시 열리지 않는 것이 확정된다.
- **제안**: 두 델리게이트 등록 직후 `AbilitySystem->HasMatchingGameplayTag(...)` 로 현재 상태를 읽어 기존 `HandleDeathTagChanged`·`HandleDialogueTagChanged` 판정 로직을 그대로 1회 호출한다.
- **확신도**: 높음

### 3. 🟡 위젯·뷰모델 인스턴스 멤버 함수에 `BlueprintCallable` 을 쓴다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`(및 `:72`, `:75`, `:78`, `:81`, `:84`, `:87`), `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:42`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리에만 허용한다. `UWxTabListWidgetBase` 의 7개 함수, `UWxButtonBase::SetButtonText`, `UWxViewModel_Ability::TryActivateAbility` 는 모두 인스턴스 멤버 함수라 대상이 아니다. (같은 모듈의 `UWxUILibrary`·`UWxMVVMConversionLibrary`·`UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 는 규칙에 부합한다.)
- **제안**: WBP 가 반드시 호출해야 하는 진입점은 `UWxUILibrary` 정적 함수로 옮기고, 순수 조회는 `BlueprintPure` 만 남기거나 Blueprint 노출을 제거한다.
- **확신도**: 높음

### 4. 🟢 이펙트 뷰모델 티커가 일시적 실패로 영구 중단될 수 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:166`
- **범주**: 버그/정확성
- **문제**: `UpdateEffectState` 는 `!ASC`(`:169`)·`!World`(`:189`) 에서 `false` 를 반환해 티커를 자기 제거하면서 `TickerHandle` 은 비우지 않는다. 티커 등록은 `Initialize` 한 곳뿐이라(`:55`) 재등록 경로가 없으므로, 월드 전환 등으로 `GetWorld()` 가 일시적으로 비는 프레임을 만나면 그 GE 가 살아 있는 동안 남은 시간·게이지 표시가 그 값에 얼어붙는다. 같은 위험을 `UWxViewModel_Ability::UpdateCooldownState` 는 주석까지 달아 핸들을 함께 비우는 방식으로 다루고 있어(`Private/MVVM/WxViewModel_Ability.cpp:331`) 두 뷰모델의 처리가 어긋나 있다.
- **제안**: 두 조기 반환에서 `TickerHandle.Reset()` 을 함께 수행해 상태를 일치시키고, `!World` 처럼 회복 가능한 실패는 `false` 로 끊지 말고 `true` 를 반환해 다음 프레임에 재시도하게 한다.
- **확신도**: 중간

### 5. 🟢 델리게이트 타입·콜백 이름이 프로젝트 접두사 규칙과 다르다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:90`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:51`
- **범주**: 규칙 위반
- **문제**: `FOnTabContentCreated`·`FOnTabContentCreatedNative`(`WxTabListWidgetBase.h:90`, `:91`)는 규칙 1 의 `FWx` 접두사를 쓰지 않는다. `FTickerDelegate` 에 바인딩되는 `UpdateCooldownState`(`WxViewModel_Ability.cpp:51`), `FlushActivationRefresh`(`WxViewModel_Ability.cpp:315`), `FlushOwnedTagsRefresh`(`WxViewModel_AbilitySystem.cpp:226`), `UpdateEffectState`(`WxViewModel_Effect.cpp:56`) 는 규칙 4 의 `Handle` 접두사를 쓰지 않는다.
- **제안**: 델리게이트 타입은 `FWxOnTabContentCreated` 계열로 개명하고, 티커 콜백은 `HandleCooldownTick` 처럼 `Handle...` 로 정리한다. `UpdateCooldownState` 처럼 다른 곳에서 직접도 호출되는 함수는 `Handle...` 래퍼를 두어 호출부 영향을 제한한다.
- **확신도**: 높음

### 6. 🟢 디자인타임 아이콘 조회가 매번 모든 Input Mapping Context 를 동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29`
- **범주**: 성능/안전
- **문제**: `GetIcon` 은 위젯 디자이너가 갱신될 때마다 Asset Registry 로 모든 `UInputMappingContext` 를 찾고(`:32`) 각 에셋을 `GetAsset()` 로 동기 로드한 뒤(`:36`) 매핑을 선형 탐색한다. 결과 캐시가 없어 IMC 수가 늘수록 WBP 디자이너 반응성이 선형으로 나빠진다. 에디터 전용(`WITH_EDITOR` + `IsDesignTime()`)이라 런타임 영향은 없다.
- **제안**: (액션, 입력 타입) 조합별 해석 결과를 정적 캐시에 담거나, 조회 대상을 프로젝트 설정에 등록된 IMC 로 좁힌다.
- **확신도**: 높음

### 7. 🟢 화면 전환 순간에 위젯 클래스를 동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:82`
- **범주**: 성능/안전
- **문제**: `PushSoftContentToLayer` 는 `LoadSynchronous()` 로 소프트 클래스를 그 자리에서 로드하며, 사망 화면(`:278`)·대화 화면(`:291`)·HUD(`Private/Component/WxHUDComponent.cpp:69`)가 모두 이 경로를 탄다. 즉 대화가 시작되는 프레임과 캐릭터가 죽는 프레임에 위젯 트리와 그 참조 에셋을 동기 로드하게 되어, 오픈월드에서 해당 클래스가 아직 메모리에 없으면 히치가 그대로 노출된다. 같은 모듈에 비동기 push 경로(`UWxAsyncAction_PushWidgetToLayer`)가 이미 있어 대비된다.
- **제안**: Experience 활성화나 대화·전투 진입 시점에 해당 클래스를 미리 스트리밍해 두거나, 사망/대화 화면 push 를 비동기 로드 경로로 옮긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 함수 주석이 동기 로드를 계약으로 명시한다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp` 및 대응 헤더
- **훑은 파일**: `Plugins/WxUI/README.md`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, 나머지 System·Component 소스와 헤더
- **미검토 / 한계**: BP/WBP 에셋의 위젯 계층·MVVM 바인딩·이벤트 그래프와 PIE 실행 검증은 범위 밖이다(클라우드 샌드박스라 엔진 없음). 확인해 위반이 **없던** 항목: `WxUI.Build.cs`·`WxUI.uplugin` 의 Wx 의존은 `WxCore` 뿐이며 소스 include 도 `WxGameplayTags.h`·`WxLocatorUtils.h` 등 WxCore 헤더만 쓴다(모듈 규칙 준수), 63개 소스 전부 첫 줄 저작권 표기가 있고, `FORCEINLINE`·규칙 위반 인라인 정의는 없다(StateTree `GetInstanceDataType()` 과 `UWxPrimaryGameLayout::PushWidgetToLayerStack` 템플릿은 예외 사유 주석이 달려 있다). `UWxViewModel_Ability` 의 충전·쿨다운 산식(진행률 분모 고정, 만료 대기 GE 취급)과 뷰모델 Outer 기반 GC 수명 모델은 따로 검토했고 결함을 찾지 못했다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 63파일 — `/module-review`로 갱신*
