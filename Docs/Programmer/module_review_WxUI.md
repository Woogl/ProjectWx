# WxUI — 코드 리뷰

> CommonUI 레이어 스택 + MVVM 골격 모듈로, 전반적으로 깔끔하고 경계(WxCore 외 Wx 미참조)와 수명 관리(구독 해제·Deinitialize 체인)가 일관되게 지켜진다. 이번 리뷰는 시스템(Subsystem/Layout)·전 MVVM 뷰모델·팝업/탭/버튼 위젯·네임플레이트의 로직(cpp)까지 깊게 봤고, 단순 데이터 헤더(UIData/DeveloperSettings)는 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 1 |

가장 먼저 볼 것:
- `UWxViewModel_Ability::UpdateCooldownState`의 `ASC->GetWorld()` 널 미검사 — 형제 코드(Effect VM)는 검사하는데 여기만 빠져 티어다운 타이밍에 크래시 가능.
- `RefreshActiveEffectViewModels`의 `Spec.Def`(GE) 널 미검사 — 같은 클래스의 Add 경로는 검사하는데 Refresh 경로만 누락.
- `BlueprintCallable` 규칙 위반(위젯/뷰모델 다수) — 일부는 MVVM/이벤트 바인딩 요건상 불가피.

## 발견

### 🟡 UpdateCooldownState에서 GetWorld() 널 미검사
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:255`
- **범주**: 버그/정확성
- **문제**: `const float WorldTime = ASC->GetWorld()->GetTimeSeconds();`가 `GetWorld()` 반환을 검사하지 않고 바로 역참조한다. 이 함수는 매 프레임 도는 티커 콜백이고, `CachedASC`는 WeakPtr라 유효해도 소유 액터가 EndPlay/파괴 진행 중이면 `GetWorld()`가 null을 반환할 수 있다. 동일 패턴의 `UWxViewModel_Effect::UpdateEffectState`(같은 티커 구조)는 `ASC->GetWorld()`를 널 검사하고 빠져나간다 — 여기만 방어가 빠져 일관성이 없다.
- **제안**: `UWorld* World = ASC->GetWorld(); if (!World) return false;` 로 가드 후 `World->GetTimeSeconds()` 사용(Effect VM과 동일 패턴).
- **확신도**: 중간

### 🟡 RefreshActiveEffectViewModels에서 GE(Spec.Def) 널 미검사
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:157`
- **범주**: 버그/정확성
- **문제**: `const UGameplayEffect* GE = Effect->Spec.Def;` 직후 `GE->FindComponent<...>()`를 널 검사 없이 호출한다. 같은 파일의 `HandleActiveEffectAdded`(line 189)는 `Spec.Def`를 명시적으로 검사하고 진행하는데, Initialize 시 호출되는 이 Refresh 경로만 검사가 빠져 있다. 활성 GE의 Def가 null인 드문 경우(비정상 스펙) 역참조 크래시.
- **제안**: `if (!GE) continue;` 한 줄 추가로 Add 경로와 대칭 맞춤.
- **확신도**: 낮음(정상 데이터에선 Def가 항상 유효 — 의도된 신뢰일 수 있음)

### 🟡 BlueprintCallable 규칙 위반 (Function Library/Async 팩토리 외 사용)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:23`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:71,78,81,84,87,90,93`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxLazyImage.h:29`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:48`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 규칙 7은 `BlueprintCallable`을 Blueprint Function Library와 Async Action 팩토리 함수에서만 허용한다. `UWxButtonBase::SetButtonText`, `UWxTabListWidgetBase`의 7개 유틸 함수는 위젯 클래스의 일반 BP 호출용이라 명확한 위반이다. (`UWxUILibrary`의 BlueprintCallable와 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer`는 규칙상 허용 대상이라 위반 아님.)
- **제안**: 순수 유틸(TabList/SetButtonText)은 함수 라이브러리로 이관하거나 지정자를 재검토한다. 단, `WxLazyImage::SetLazyTexture`(1-arg MVVM setter 바인딩 대상)와 `WxViewModel_Ability::TryActivateAbility`(WBP OnClicked 이벤트 대상)는 MVVM/BP 바인딩이 BlueprintCallable UFUNCTION을 요구하므로 예외로 문서화하는 편이 현실적이다.
- **확신도**: 중간(규칙상 위반이나 일부 항목은 바인딩 요건상 불가피)

### 🟡 네임플레이트 매 틱 전체 재계산 (숨김 상태에서도)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31`, `:95`
- **범주**: 성능/안전
- **문제**: `TickComponent`가 매 프레임 `RefreshVisibility()`(내부에서 `GetOwnedGameplayTags`로 태그 컨테이너를 매 틱 복사)와 카메라 거리 스케일을 수행한다. 위젯이 숨김(`SetVisibility(false)`)일 때도 거리·스케일 계산이 계속 돈다. 적 다수가 동시에 존재하면 네임플레이트마다 프레임당 컨테이너 복사 + PC 조회 + 거리 계산이 누적된다.
- **제안**: 표시 여부는 ASC 태그 이벤트(`RegisterGenericGameplayTagEvent`, VM들이 쓰는 패턴) 구독으로 전환해 매 틱 `GetOwnedGameplayTags` 복사를 없애고, 스케일 계산은 현재 표시 중일 때만 수행한다.
- **확신도**: 중간(거리 스케일은 틱이 필요 — 부분적으로 의도된 설계일 수 있음)

### 🟢 쿨다운 티커 중 매 틱 발동가능 재평가
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:303`, `:308`
- **범주**: 성능/안전
- **문제**: `UpdateCooldownState`가 쿨다운 진행 중 매 틱 `RefreshActivationState()`를 호출하고, 이 함수는 `GetActivatableAbilities()` 전체를 선형 탐색 후 `CanActivateAbility`/`CheckCost`를 호출한다. 동시에 티커 본문도 `GetActiveEffects(Query)`를 스캔한다. 쿨다운 중인 어빌리티 VM이 여러 개면 프레임당 ASC 스캔이 중첩된다. (쿨다운 활성 구간에만 도는 것으로 게이팅돼 있어 영향은 제한적.)
- **제안**: 충전 수/발동가능 상태가 실제로 바뀐 프레임에만 재평가하도록 이전 값과 비교해 조기 반환하거나, 티커 주기를 프레임보다 늘린다.
- **확신도**: 낮음(의도된 설계 — 충전 회복은 이벤트가 없어 폴링이 불가피)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_InteractionList.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxLazyImage.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Selection.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, 관련 Public 헤더 전반, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`
- **미검토 / 한계**: BP/WBP 내부(위젯 계층·MVVM 바인딩 그래프)는 리뷰 범위 밖. `FieldNotify` 프로퍼티만 있는 단순 뷰모델 헤더(Character/Effect/Attribute/Selection 등)의 Getter/Setter 보일러플레이트는 로직 없어 정독하지 않음. 규칙 위반 항목의 "MVVM 바인딩이 BlueprintCallable을 강제하는지"는 엔진 MVVM 픽커 동작 근거로 판단했으며 런타임 확인은 하지 않음.

---
*문서 기준 커밋 `702fc70f` · 리뷰일 2026-07-22 · 소스 56파일 — `/module-review`로 갱신*
