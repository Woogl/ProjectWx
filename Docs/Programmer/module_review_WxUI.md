# WxUI — 코드 리뷰

> CommonUI 레이어 스택 + MVVM 골격으로 구성된 UI 도메인 플러그인. 전반적으로 방어적 코딩·수명주기 관리·모듈 경계 준수가 좋은 편이다. 시스템 오케스트레이터(`UWxUIManagerSubsystem`), 레이어 스택(`UWxPrimaryGameLayout`), 전 MVVM 뷰모델, 주요 위젯(팝업/탭/버튼/액션/네임플레이트)의 C++ 를 통독했다. 심각한 결함은 없고, 널 가드 누락 1건과 규칙 위반 2건이 주된 개선점이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 활성 GE 순회 시 `Spec.Def` 널 가드 누락
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:157`
- **범주**: 버그/정확성
- **문제**: `RefreshActiveEffectViewModels` 에서 `const UGameplayEffect* GE = Effect->Spec.Def;` 직후 널 검사 없이 `GE->FindComponent<...>()`(158행)를 호출한다. 같은 파일의 이벤트 경로 `HandleActiveEffectAdded`(189행)는 `if (!Spec.Def) return;` 로 명시적으로 방어하는데, 이 순회 경로만 방어가 빠져 있어 처리 방식이 비대칭이다. `Spec.Def` 가 널인 활성 GE(비정상/전이 상태)를 만나면 널 역참조로 크래시한다.
- **제안**: 158행 진입 전에 `if (!GE) { continue; }` 를 추가해 `HandleActiveEffectAdded` 와 방어 수준을 맞춘다.
- **확신도**: 중간(활성 GE 의 `Spec.Def` 가 널인 경우는 드물지만, 동일 모듈 내 다른 경로가 방어하는 값을 여기서만 신뢰하는 것은 명백한 비대칭이다).

### 2. 🟡 `UWxViewModel_InteractionList::Deinitialize` 가 `Super::Deinitialize()` 미호출
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_InteractionList.cpp:12`
- **범주**: 규칙 위반 (CLAUDE.md 코딩규칙 5 — override 시 `Super::` 호출)
- **문제**: `Deinitialize()` 는 `UWxViewModel::Deinitialize` 의 override(헤더 27행 `virtual ... override`)인데 본문에서 `Super::Deinitialize()` 를 호출하지 않는다. 다른 모든 VM(`WxViewModel_Attribute`·`_Effect`·`_Ability`·`_Character`·`_AbilitySystem`)은 마지막에 `Super::Deinitialize()` 를 호출한다. 현재 베이스 구현이 비어 있어 동작상 무해하지만, 규칙 위반이며 향후 베이스에 공통 정리 로직이 추가되면 이 VM 만 누락된다.
- **제안**: 함수 말미에 `Super::Deinitialize();` 를 추가한다.
- **확신도**: 높음.

### 3. 🟡 `BlueprintCallable` 을 Function Library/Async 팩토리 외에서 사용
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:48` (`TryActivateAbility`), `Plugins/WxUI/Source/WxUI/Public/Widget/WxLazyImage.h:29` (`SetLazyTexture`), `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:23` (`SetButtonText`), `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:71,78,81,84,87,90,93`
- **범주**: 규칙 위반 (CLAUDE.md 코딩규칙 7 — `BlueprintCallable` 은 Blueprint Function Library·Blueprint Async Action 팩토리 함수에서만)
- **문제**: 위 지정자들은 ViewModel/위젯 클래스의 멤버 함수에 붙어 있어 규칙 7 의 허용 범위를 벗어난다. (`WxUILibrary.h` 의 `BlueprintCallable` 은 Function Library 라 허용, `WxAsyncAction_PushWidgetToLayer.h:27` 은 Async 팩토리라 허용 — 위반 아님.)
- **제안**: 규칙을 엄격히 따른다면 이 함수들을 `BlueprintFunctionLibrary` 로 이전하거나, MVVM 바인딩/위젯 API 관례상 BP 노출이 불가피하다면 규칙 7 에 "위젯/뷰모델의 MVVM 바인딩 타깃" 예외 조항을 명문화해 규칙과 코드를 일치시킨다.
- **확신도**: 중간(문언상 위반은 분명하나, 이들은 WBP MVVM 바인딩·CommonUI 위젯 API 를 전제로 설계된 함수라 의도된 설계로 보이며 실무상 이전이 부자연스럽다).

### 4. 🟡 쿨다운 티커에서 `ASC->GetWorld()` 널 검사 누락
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:255`
- **범주**: 성능/안전
- **문제**: `UpdateCooldownState` 는 `const float WorldTime = ASC->GetWorld()->GetTimeSeconds();` 로 `GetWorld()` 결과를 널 검사 없이 역참조한다. 동일 성격의 `WxViewModel_Effect::UpdateEffectState`(173행)와 `Initialize`(42행)는 `const UWorld* World = InASC->GetWorld(); if (!World) { return ...; }` 로 방어한다. 소유 액터 파괴 도중 티커가 한 번 더 도는 등의 상황에서 `GetWorld()` 가 널이면 크래시 가능성이 있다.
- **제안**: `Effect` VM 과 동일하게 `GetWorld()` 를 지역 변수로 받아 널 검사 후 사용한다.
- **확신도**: 낮음(티커 진입 시 `CachedASC` 유효성은 확인되며 유효 ASC 의 `GetWorld()` 가 널인 경우는 드물다. 다만 자매 VM 과의 방어 비대칭은 실재한다).

### 5. 🟢 네임플레이트가 숨김 상태에서도 매 틱 스케일 계산
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31`
- **범주**: 성능/안전
- **문제**: `TickComponent` 는 `RefreshVisibility()` 후 위젯 존재 여부만 보고, 위젯이 숨김(`SetVisibility(false)`) 상태여도 `GetPlayerViewPoint`·거리·`SetRenderScale` 을 매 틱 수행한다. 화면에 다수의 네임플레이트가 있고 대부분 숨김일 때(기본 정책은 비전투 시 숨김) 불필요한 매 틱 연산이 누적된다.
- **제안**: `RefreshVisibility()` 결과가 숨김이면 스케일 계산을 건너뛰고 조기 반환한다.
- **확신도**: 중간(개별 비용은 작지만 개체 수에 비례. 의도적 단순화일 수 있음).

### 6. 🟢 로컬 플레이어 제거 이벤트 미처리
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:50`
- **범주**: 설계/구조
- **문제**: `Initialize` 는 `OnLocalPlayerAddedEvent` 만 구독하고 `OnLocalPlayerRemovedEvent` 는 구독하지 않는다. 로컬 플레이어가 런타임에 제거되면 해당 플레이어의 `OnPlayerControllerChanged` 바인딩과 레이아웃 정리 시점이 `Deinitialize` 까지 미뤄진다. 단일 플레이어에서는 무해하나 분할화면/로컬 멀티에서 잔여 바인딩이 남는다.
- **제안**: 로컬 멀티를 지원할 계획이면 `OnLocalPlayerRemovedEvent` 를 구독해 대칭적으로 정리한다.
- **확신도**: 낮음(현재 단일 플레이어 전제로 보이며 의도된 범위 제한일 수 있음).

### 7. 🟢 쿨다운 지속시간을 최초 1회만 캐시
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:218`
- **범주**: 버그/정확성
- **문제**: `HandleGameplayEffectApplied` 는 `if (SpecDuration > 0.f && CooldownDuration <= 0.f)` 조건으로 `CooldownDuration` 을 최초 1회만 설정한다. 이후 같은 어빌리티의 쿨다운 지속시간이 (버프/디버프로) 달라져도 `CooldownDuration` 은 갱신되지 않아 `UpdateCooldownState` 의 `CooldownPercent = NextChargeRemaining / CooldownDuration` 이 실제와 어긋날 수 있다.
- **제안**: 쿨다운 지속시간이 가변인 설계라면 적용 시마다 최신 `SpecDuration` 으로 갱신한다. 고정 지속시간 전제라면 주석으로 명시한다.
- **확신도**: 낮음(고정 쿨다운을 전제한 의도된 캐싱일 가능성이 높다).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_InteractionList.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`
- **훑은 파일**: `WxUILibrary.cpp`, `WxMVVMConversionLibrary.cpp`, `WxViewModel.cpp`, `WxViewModel_Character.cpp`, `WxViewModel_Selection.cpp`, `WxViewModel_Interaction.cpp`, `WxButtonBase.cpp`, `WxActionWidget.cpp`, `WxLazyImage.cpp`, `WxTabButtonBase.cpp`, `WxGamePopup.cpp`, `WxActivatableWidget.cpp`, `WxEffectComponent_UIData.cpp`, `WxUI.Build.cs`, 주요 Public 헤더들
- **미검토 / 한계**: `WxUIDeveloperSettings.cpp`·`WxUIModule.cpp` 등 단순 정의/등록 파일은 표면만 확인. WBP 위젯 계층·MVVM 바인딩 그래프는 이 리뷰 범위 밖(C++ 근거로만 판단). 모듈 경계(WxCore 외 Wx 참조 금지)는 `Build.cs`·include 확인 결과 위반 없음.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 56파일 — `/module-review`로 갱신*
