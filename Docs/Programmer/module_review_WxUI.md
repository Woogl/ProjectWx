# WxUI — 코드 리뷰

> CommonUI 레이어 스택 + MVVM 뷰모델 골격이 잘 정돈돼 있고, 이전 리뷰(2026-07-24)에서 지적된 아이콘 동기 로드·널 가드 누락·자식 VM Deinitialize 미전파·네임플레이트 표시 판정 매 틱 재계산은 모두 해소됐다. 남은 결함은 쿨다운 VM의 티커 수명 관리와 초기 상태 시딩, 그리고 이벤트 구동 재평가의 비용에 몰려 있으며 🔴는 없다. 이번 리뷰는 MVVM 뷰모델 전 계층·UIManager/Layout·Nameplate·TabList의 cpp를 깊게 보고, 나머지 위젯·라이브러리·헤더는 훑었다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 티커 조기 종료 시 `TickerHandle`이 남아 쿨다운 갱신이 영구 정지된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:284-297` (`UpdateCooldownState` 조기 반환) · 같은 파일 `263-268` (재등록 게이트)
- **범주**: 버그/정확성
- **문제**: `UpdateCooldownState`가 `!ASC || !CachedCooldownClass || CooldownDuration <= 0.f`(287) 또는 `!World`(295)로 `return false`할 때 `TickerHandle.Reset()`을 하지 않는다. `false` 반환은 티커를 제거하지만 핸들은 유효한 값으로 남고, `HandleGameplayEffectApplied`는 `if (!TickerHandle.IsValid())`(263)로만 재등록 여부를 판단하므로 이후 어떤 쿨다운 GE가 적용돼도 티커가 다시 붙지 않는다. 정상 만료 경로(329-339)만 `Reset()`을 부른다.
  구체적 실패: 쿨다운 GE의 DurationPolicy가 Infinite이거나 `SpecApplied.GetDuration()`이 0 이하면 `CooldownDuration`이 0으로 남은 채(256) `SetIsOnCooldown(true)`(261) + 티커 등록(265)이 일어나고, 첫 틱에서 287 조건으로 즉시 빠져나간다. 결과적으로 그 어빌리티는 `IsOnCooldown = true`, `CooldownRemaining/Percent`가 초기값 그대로 고정되며 이후 영구히 복구되지 않는다. 레벨 전환 중 `World`가 잠깐 null이 되는 경우도 같은 상태로 빠진다.
- **제안**: 두 조기 반환 앞에서 `TickerHandle.Reset()`을 호출하거나(정상 만료 경로와 동일), 재등록 게이트를 핸들 유효성이 아니라 `IsOnCooldown`/`ConsumedCharges` 같은 상태로 바꾼다.
- **확신도**: 높음 (코드 결함은 확정. 발현 빈도는 쿨다운 GE 저작에 의존)

### 2. 🟡 어빌리티 VM이 이미 진행 중인 쿨다운을 반영하지 않고 초기화된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:24-33`, `59`
- **범주**: 버그/정확성
- **문제**: `Initialize`는 `SetCurrentCharges(AbilityMaxRecharges)` / `IsOnCooldown = false`(기본값)로만 시작하고, ASC에 이미 붙어 있는 활성 쿨다운 GE를 조회하지 않는다. VM은 `GetOrCreateAbilityViewModel`로 UI 바인딩 시점에 지연 생성되므로(`WxViewModel_AbilitySystem.cpp:128-159`), 쿨다운 중에 HUD/패널이 새로 만들어지면(PC 교체로 레이아웃 재생성, 리스폰, 메뉴 최초 오픈) 그 어빌리티는 "충전 만땅·쿨다운 없음"으로 표시된다. 티커는 다음 쿨다운 GE 적용 전까지 돌지 않으므로 자가 교정도 되지 않는다(`RefreshActivationState`가 `CanActivate=false`로 만들어 부분적으로만 가려준다).
- **제안**: `Initialize` 말미에서 `UpdateCooldownState(0.f)`를 1회 실행하고, 반환값이 true면 티커를 등록한다(활성 쿨다운 GE 스캔 로직을 그대로 재사용).
- **확신도**: 중간

### 3. 🟡 런타임 제거 경로가 자식 Effect VM의 `Deinitialize()`를 건너뛴다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:236-244` (`HandleActiveEffectRemoved`) · 같은 파일 `169` (`RefreshActiveEffectViewModels`의 `Empty()`)
- **범주**: 설계/구조
- **문제**: 같은 파일 `38-60`의 `Deinitialize`는 "배열에서만 떼면 자식이 GC까지 티킹·구독을 유지한다"는 이유로 자식 VM에 명시적으로 `Deinitialize()`를 전파하는데, 정작 런타임에 가장 자주 도는 제거 경로인 `HandleActiveEffectRemoved`는 `RemoveAt`만 하고 전파하지 않는다. `RefreshActiveEffectViewModels`의 `Empty()`도 동일하다(현재는 `Initialize`에서만 호출돼 배열이 비어 있지만, 주석상 재호출 가능한 API로 설계돼 있다). 버려진 `UWxViewModel_Effect`는 `IconStreamHandle`(`WxViewModel_Effect.h:97`)을 쥔 채 GC의 `BeginDestroy`까지 남아 텍스처 스트리밍 참조를 붙잡고, 티커도 다음 틱까지 한 번 더 돈다.
- **제안**: `RemoveAt`/`Empty()` 직전에 대상 VM의 `Deinitialize()`를 호출해 `Deinitialize` 경로와 동일한 결정적 정리를 보장한다.
- **확신도**: 중간

### 4. 🟡 어빌리티 발동 가능 재평가가 제네릭 태그 이벤트와 매 틱 GE 질의에 얹혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:44`, `300-327`, `346`, `357-385`
- **범주**: 성능/안전
- **문제**: (a) VM마다 `RegisterGenericGameplayTagEvent()`를 구독해(44) ASC의 **모든** 태그 변경 1회당 `RefreshActivationState`를 돌린다. 이 함수는 `GetActivatableAbilities()` 선형 탐색 + `CanActivateAbility` + `CheckCost`(비용 GE 평가)를 수행한다. 전투 중 태그 변경은 프레임당 수 회 발생하므로 비용이 `VM 수 × 태그 변경 수`로 곱해진다. (b) `UpdateCooldownState`는 쿨다운이 도는 동안 매 틱 `ASC->GetActiveEffects(Query)`로 `TArray`를 새로 할당·반환받아 전체 활성 GE를 훑고(307), 그 위에 다시 `RefreshActivationState`를 부른다(346).
- **제안**: 태그 이벤트는 해당 어빌리티의 요건 태그(`ActivationRequiredTags`/`ActivationBlockedTags`)에 대한 `RegisterGameplayTagEvent` 단위 구독으로 좁히고, 매 틱 재평가는 프레임 단위 dirty 플래그로 합치거나 갱신 주기를 늘린다(UI 표시에 60Hz 정확도는 불필요).
- **확신도**: 중간 (측정 없이 코드 형태로만 판단 — 실제 VM 수/태그 트래픽에 좌우됨)

### 5. 🟡 네임플레이트가 숨김 상태에서도 매 틱 스케일을 갱신한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31-60`, `12` (`bCanEverTick = true`), `18` (기본 숨김)
- **범주**: 성능/안전
- **문제**: 표시 판정은 태그 이벤트 구동으로 개선됐지만, `TickComponent`는 컴포넌트 가시성과 무관하게 매 틱 `GetFirstPlayerController()` → `GetPlayerViewPoint()` → `SetRenderScale()`을 수행한다. 네임플레이트는 기본 숨김(18)이라 월드의 적 대부분이 "보이지 않는 상태로 매 프레임 렌더 트랜스폼을 갱신"한다(`SetRenderScale`은 위젯 무효화를 유발한다). 적 수에 비례해 낭비가 커진다.
- **제안**: `RefreshVisibility`에서 결과에 맞춰 `SetComponentTickEnabled(bVisible)`을 함께 토글한다(표시 상태 진실이 이미 한 곳에 모여 있으므로 추가 상태가 필요 없다). 거리 컬링 상한을 두는 것도 같은 지점에서 가능하다.
- **확신도**: 중간

### 6. 🟡 `PrimaryGameLayout`이 단일 필드라 로컬 플레이어가 둘 이상이면 앞 플레이어의 레이아웃이 파괴된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:45-48`, `220-237` · `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h:75-76`
- **범주**: 설계/구조
- **문제**: `Initialize`가 모든 로컬 플레이어에 대해 `HandleLocalPlayerAdded`를 돌리고(45-48), 각자의 `HandlePlayerControllerSet`이 무조건 기존 `PrimaryGameLayout`을 `RemoveFromParent()` 후 자기 것으로 교체한다(225-236). 필드가 하나뿐이라 스플릿스크린/로컬 co-op에서는 마지막 플레이어의 레이아웃만 남고 앞 플레이어는 UI가 사라진다. `UWxUILibrary::GetPrimaryGameLayout`(`WxUILibrary.cpp:42`)도 플레이어 구분 없이 이 하나를 돌려준다. README는 "플레이어별 레이아웃 생성"이라고 기술하고 있어 문서와 구현이 어긋난다.
- **제안**: 싱글 로컬 플레이어 전제가 확정이라면 `HandleLocalPlayerAdded`를 첫 로컬 플레이어로 제한하고 헤더 주석·README를 그 전제로 정정한다. 아니라면 `TMap<ULocalPlayer*, UWxPrimaryGameLayout*>`로 승격하고 조회 API에 플레이어 인자를 추가한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 오픈월드 싱글 전제라면 문서 정정만으로 충분)

### 7. 🟢 override 2곳이 `Super::`를 호출하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:22` (`Activate`) · `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:108` (`HandleTabCreation_Implementation`)
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 규칙 5는 override 시 `Super::` 호출을 요구한다. 두 베이스 구현은 현재 비어 있어 동작 차이는 없지만, 값 전체를 대체하는 `GetDesiredInputConfig`(이전 리뷰에서 예외로 판정)와 달리 여기서는 `Super::` 호출이 무해하고 규칙과도 일치한다.
- **제안**: 각 함수 선두에 `Super::Activate();` / `Super::HandleTabCreation_Implementation(TabId, TabButton);`을 추가한다.
- **확신도**: 높음

### 8. 🟢 `HandleTabCreation_Implementation`의 `TabButton` 널 검사 순서가 뒤집혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:122`, `131`
- **범주**: 버그/정확성
- **문제**: 122줄이 `TabButton->GetClass()`를 검사 없이 역참조하는데, 정작 131줄에서는 `if (TabButtonContainer && TabButton)`으로 널을 방어한다. 같은 함수 안에서 같은 포인터에 대한 가정이 상충한다(방어가 필요하다면 122줄이 먼저 터진다).
- **제안**: 함수 진입부에서 `if (!TabButton) { return; }`으로 한 번에 정리하고 131줄의 중복 검사를 제거한다.
- **확신도**: 높음 (엔진 경로에서 널이 오지 않을 가능성이 크지만 일관성 결함은 확정)

### 9. 🟢 쿨다운 퍼센트만 클램프되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:342`
- **범주**: 버그/정확성
- **문제**: `SetCooldownPercent(NextChargeRemaining / CooldownDuration)`에 상한이 없다. `CooldownDuration`은 첫 적용 시에만 기록되고(256-259) 충전이 모두 회복될 때까지 갱신되지 않으므로, 도중에 더 긴 쿨다운이 적용되면 1을 초과한 값이 프로그레스 바에 전달된다. 동일 패턴인 `UWxViewModel_Effect::UpdateEffectState`는 `FMath::Min(..., 1.f)`로 클램프한다(`WxViewModel_Effect.cpp:209`).
- **제안**: Effect VM과 동일하게 `FMath::Clamp(..., 0.f, 1.f)`를 적용한다.
- **확신도**: 중간

### 10. 🟢 `InitializeViewModels`가 ASC 널을 검증하지 않고 재호출도 방어하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:91-97`
- **범주**: 버그/정확성
- **문제**: 위젯/`UMVVMView` 유효성은 조기 반환으로 방어하면서(74-84) `InASC`는 검사 없이 94줄에서 역참조한다. 또 같은 컴포넌트에 두 번 호출하면 `RegisterGenericGameplayTagEvent()`에 중복 바인딩되고 VM도 새로 생성된다(기존 VM은 `Deinitialize` 없이 버려진다). 현재 유일한 호출부(`Source/WxGame/Character/WxEnemyCharacter.cpp:50`)는 생성자 서브오브젝트 ASC를 1회만 넘기므로 발현하지 않지만, 플러그인 공개 API라 소비 측이 늘면 노출된다.
- **제안**: 선두에 `if (!InASC) { return; }`를 추가하고, 재초기화 시 기존 구독을 `RemoveAll(this)`로 떼고 이전 VM에 `Deinitialize()`를 호출한다.
- **확신도**: 중간

### 11. 🟢 `EffectEndTime`은 계산만 하고 아무도 읽지 않는 데드 필드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h:89` · `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:71`
- **범주**: 중복/복잡도
- **문제**: `Initialize`에서 `EffectEndTime = CurrentTime + Remaining`을 기록하지만 `UpdateEffectState`는 `ActiveEffect->StartWorldTime + CachedDuration`으로 매번 다시 계산하며 이 필드를 읽지 않는다. 또 `Deinitialize`가 `EffectEndTime`/`CachedDuration`/`PendingIcon`을 되돌리지 않아 "남은 값"의 의미가 모호하다.
- **제안**: 필드를 제거하거나(권장), 남긴다면 `UpdateEffectState`가 이 값을 진실로 사용하도록 통일하고 `Deinitialize`에서 초기화한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxLazyImage.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Selection.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, 및 대응 Public 헤더 전반
- **미검토 / 한계**:
  - BP/WBP 내부(위젯 계층·MVVM 바인딩 그래프·디폴트값)와 `Plugins/WxUI/Content/`는 범위 밖.
  - 모듈 경계는 정적으로 확인 완료 — `.uplugin`·`Build.cs`·include 모두 Wx 중 `WxCore`만 참조하며, 전 소스 파일이 Copyright 첫 줄을 지킨다(위반 0건).
  - 위젯 헬퍼의 `BlueprintCallable`(`WxButtonBase.h:23`, `WxTabListWidgetBase.h:71-93`, `WxLazyImage.h:29`, `WxViewModel_Ability.h:50`)은 이전 리뷰에서 "의도된 위젯/VM 관용 패턴"으로 판정됐고 그 이후 변화가 없어 재보고하지 않았다. 판정을 뒤집으려면 규칙 7의 예외를 CLAUDE.md에 명문화하는 편이 낫다.
  - 성능 항목(4·5)은 코드 형태 기반 추론이며 프로파일링은 수행하지 않았다. CommonUI 스택의 위젯 풀링/전이 타이밍과 MVVM 바인딩 재평가 시점은 엔진 정상 동작으로 가정했다.
  - 멀티플레이/스플릿스크린 실행 검증 없음(6번은 정적 판단).

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 53파일 — `/module-review`로 갱신*
