# WxUI — 코드 리뷰

> CommonUI 레이어 스택 + MVVM 뷰모델 골격으로, 전반적으로 잘 정돈돼 있고 수명 관리(Deinitialize/구독 해제)와 디커플링(도메인 타입 미참조, 태그/엔진 타입만 노출) 규약이 일관되게 지켜진다. 치명적 결함(🔴)은 없다. 이번 리뷰는 System(Manager/Layout)·MVVM 뷰모델 전 계층·AsyncAction·Nameplate·주요 위젯 베이스의 cpp를 깊게 보고, 나머지 위젯·라이브러리·헤더는 훑어 확인했다. BP/WBP 내부(위젯 계층·바인딩 그래프)는 범위 밖이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 Effect 뷰모델이 아이콘을 동기 로드해 규약·성능에서 벗어남
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:26`
- **범주**: 성능/안전
- **문제**: `InUIData->Icon.LoadSynchronous()` 로 이펙트 아이콘을 게임 스레드에서 동기 로드한다. GE가 적용될 때(전투 중) 아이콘 텍스처가 아직 스트리밍되지 않았다면 그 자리에서 로드 히치가 발생한다. 같은 목적의 `UWxViewModel_Ability::SetIconSoft`(`WxViewModel_Ability.cpp:214`)는 `RequestAsyncLoad`로 비동기 처리하고, README도 「VM 은 `LoadSynchronous` 미호출」을 규약으로 명시하고 있어 이 한 곳만 규약을 벗어난다.
- **제안**: Ability VM과 동일하게 소프트 참조를 비동기 스트리밍하거나(취소 핸들 포함), 아이콘을 `TSoftObjectPtr`로 그대로 노출해 View 측 `UWxLazyImage`/`UCommonLazyImage`가 로드하도록 위임한다.
- **확신도**: 높음

### 2. 🟡 티커/리프레시 경로의 가드 없는 널 역참조(형제 경로엔 가드 존재)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:157-158` · `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:293`
- **범주**: 버그/정확성
- **문제**: 두 곳이 형제 경로에는 있는 널 가드를 빠뜨렸다. (a) `RefreshActiveEffectViewModels`는 `const UGameplayEffect* GE = Effect->Spec.Def;` 직후 `GE->FindComponent<...>()`를 호출하는데 `GE`를 검사하지 않는다. 반면 같은 데이터를 다루는 `HandleActiveEffectAdded`(같은 파일 189줄)는 `Spec.Def`를 널 체크한다. (b) `UpdateCooldownState`는 `ASC->GetWorld()->GetTimeSeconds()`로 `GetWorld()` 결과를 검사 없이 역참조하는데, 동일 패턴의 `UWxViewModel_Effect::UpdateEffectState`(`WxViewModel_Effect.cpp:173-177`)는 `World` 널을 가드한다. 레벨 전환/월드 teardown 중 티커가 살아있는 짧은 구간에서 크래시 가능성이 있다.
- **제안**: (a) `if (!GE) continue;` 추가. (b) `UWorld* World = ASC->GetWorld(); if (!World) return false;`로 형제 경로와 동일하게 가드.
- **확신도**: 중간 (정상 경로에선 대부분 유효하나, 형제 경로가 이미 방어하고 있어 일관성·teardown 안전 측면에서 보강 가치가 있음)

### 3. 🟡 AbilitySystem VM의 Deinitialize가 자식 VM에 전파되지 않음
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:39-42`
- **범주**: 설계/구조
- **문제**: `Deinitialize`가 `AttributeViewModels`/`AbilityViewModels`/`ActiveEffectViewModels`를 `Empty()`만 하고 각 자식 VM의 `Deinitialize()`는 호출하지 않는다. 부모인 `UWxViewModel_Character::Deinitialize`는 자식(`AbilitySystem`)에 명시적으로 전파(`WxViewModel_Character.cpp:27`)하는 것과 대비된다. 결과적으로 배열에서 떨어져 나온 자식 Effect/Ability VM은 GC가 `BeginDestroy→Deinitialize`를 부를 때까지 FTSTicker 티커와 ASC 델리게이트 구독을 유지한 채로 남아, 그 사이 ASC 이벤트에 반응하고 매 프레임 티킹한다. `BeginDestroy` 경로 덕분에 크래시나 영구 누수는 아니지만(수명 안전은 확보됨), 결정적 teardown이 아니며 형제 클래스와 일관되지 않는다.
- **제안**: `Empty()` 전에 각 자식 VM에 `Deinitialize()`를 호출해 티커·구독을 즉시 정리한다(Character VM 패턴과 동일).
- **확신도**: 중간

### 4. 🟡 네임플레이트가 매 틱 태그 컨테이너를 할당하고 PC를 조회
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31-98`
- **범주**: 성능/안전
- **문제**: `TickComponent`가 네임플레이트마다 매 틱 `RefreshVisibility`를 돌리고, 그 안에서 `ASC->GetOwnedGameplayTags(OwnedTags)`로 태그 컨테이너를 새로 채운다(힙 할당·복사). 또 매 틱 `World->GetFirstPlayerController()`를 조회한다. 화면에 적이 다수일 때 프레임당 N회 할당이 쌓인다. 스케일 계산(거리 기반)은 카메라가 움직이므로 매 틱이 타당하지만, 표시 여부는 태그가 바뀔 때만 갱신하면 충분하다.
- **제안**: 표시 판정은 `ASC->RegisterGenericGameplayTagEvent()` 구독(VM들이 이미 쓰는 패턴)으로 이벤트 구동화하고, 틱에서는 스케일만 갱신한다. PC/뷰포인트는 필요 시 캐시한다.
- **확신도**: 중간 (`SetVisibility`가 동일값 no-op이라 시각적 문제는 없으나, 다수 엔티티에서 할당 비용은 실재)

### 5. 🟡 위젯 헬퍼의 BlueprintCallable 이 규칙 7과 충돌
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:23` · `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:71,78,81,84,87,90,93` · `Plugins/WxUI/Source/WxUI/Public/Widget/WxLazyImage.h:29`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 규칙 7은 `BlueprintCallable`을 「Blueprint Function Library·Blueprint Async Action 팩토리 함수에서만」 허용한다. 위 위젯 베이스들의 세터/탭 관리 헬퍼는 그 두 범주가 아니다(WBP 그래프에서 호출할 목적). `UWxUILibrary`(BP Function Library), `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer`(Async 팩토리)는 규칙에 부합하고, `UWxViewModel_Ability::TryActivateAbility`는 프로젝트가 이미 인정한 VM 커맨드 예외에 해당한다. 남는 것이 이 위젯 헬퍼들로, CommonUI/Lyra 유래의 관용 패턴이라 의도된 것으로 보이나 문언상으로는 규칙 7 위반이다.
- **제안**: 위젯 헬퍼 `BlueprintCallable`을 규칙의 명시적 예외로 문서화(VM 커맨드처럼)하거나, WBP에서 직접 호출이 불필요한 것은 지정자를 제거해 규칙과 코드를 일치시킨다.
- **확신도**: 낮음 (의도된 위젯 관용 패턴일 가능성이 높음 — 판단은 사람이)

### 6. 🟢 GetDesiredInputConfig override가 Super:: 를 호출하지 않음
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp:5-16`
- **범주**: 규칙 위반
- **문제**: 규칙 5는 override 시 `Super::` 호출을 요구하나 이 override는 값을 완전 대체 산출하며 `Super::`를 부르지 않는다. `default` 분기가 베이스 기본값과 동일한 미설정 `TOptional`을 반환하므로 동작상 문제는 없고 `Super::` 호출이 오히려 결과를 덮어써 무의미하다. 규칙 문언과의 미세한 간극이라, 미래 세션이 기계적으로 「고치려」다 회귀를 넣지 않도록 기록만 남긴다.
- **제안**: 현행 유지 권장. 규칙 5의 「값 완전 대체 override」 예외를 인지.
- **확신도**: 낮음(의도된 설계)

### 7. 🟢 티커 콜백에 Handle prefix 부재(규칙 6 경계)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:284`(`UpdateCooldownState`) · `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:152`(`UpdateEffectState`)
- **범주**: 규칙 위반
- **문제**: 규칙 6은 델리게이트 바인딩 콜백에 `Handle` prefix를 요구한다. 이 둘은 `FTickerDelegate`(델리게이트)에 바인딩되지만 `Handle`을 쓰지 않는다. 같은 모듈에서 `FStreamableDelegate` 콜백은 `HandleIconLoaded`/`HandleWidgetClassLoaded`로 prefix를 지킨다. 다만 이벤트 콜백이 아니라 매 프레임 tick 루프(반환 bool로 지속 제어)라 의도적으로 구분한 것으로 보인다.
- **제안**: 규칙을 엄격 적용하려면 `HandleCooldownTick`/`HandleEffectTick` 등으로 통일하고, 아니면 「tick 루프는 예외」를 규약에 명문화한다.
- **확신도**: 낮음(tick 루프와 이벤트 콜백을 구분한 의도로 보임)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxLazyImage.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Selection.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, 및 대응 Public 헤더 전반
- **미검토 / 한계**: BP/WBP 내부(위젯 계층·MVVM 바인딩 그래프·디폴트값)는 리뷰 범위 밖. MVVM 매크로(`UE_MVVM_SET_PROPERTY_VALUE`/`BROADCAST_FIELD_VALUE_CHANGED`)가 생성하는 FieldNotify 배선은 엔진 정상 동작으로 가정. 티커/GC 상호작용(자식 VM orphan 구간)은 정적 분석 기준 추론이며 런타임 프로파일링은 수행하지 않음.

---
*문서 기준 커밋 `efc26014` · 리뷰일 2026-07-24 · 소스 53파일 — `/module-review`로 갱신*
