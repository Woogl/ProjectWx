# WxUI — 코드 리뷰

> 레이어 스택·MVVM·인디케이터 세 축의 수명·구독 해제 규약이 주석으로 명시된, 전반적으로 건강한 모듈이다. 직전 리뷰(2026-07-25)의 티커 핸들 미초기화(🟡 1번)는 해소됐고, 이번에 새로 들어온 인디케이터 캔버스에서 표시 복구 불가 결함을 찾았다. 이번 리뷰는 `UWxUIManagerSubsystem`·VM 계층 전체·`SWxIndicatorCanvas`·`UWxHUDLayout`을 cpp까지 깊게 보고, 위젯 베이스·설정·모듈 파일은 훑었다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 인디케이터를 전체 숨김 처리하면 표시가 영구히 복구되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Indicator/SWxIndicatorCanvas.cpp:391-397` · `Plugins/WxUI/Source/WxUI/Private/Indicator/SWxIndicatorCanvas.h:60-68`
- **범주**: 버그/정확성
- **문제**: `SetShowAnyIndicators(false)`는 슬롯의 `bIsIndicatorVisible` 플래그를 건드리지 않고 자식 위젯의 Slate Visibility만 `Collapsed`로 직접 덮어쓴다. 반대로 유일한 복구 경로인 `HandleActiveTimer`의 `CurrentSlot.SetIndicatorVisible(true)`(`SWxIndicatorCanvas.cpp:282`)는 `bIsIndicatorVisible != bInVisible`일 때만 위젯 Visibility를 만지므로, 플래그가 `true`인 채 위젯만 Collapsed가 된 슬롯은 영구 no-op이 된다. `OnArrangeChildren`도 `ArrangedChildren.Accepts()`(`:68`)에서 걸러 배치조차 하지 않는다. 트리거는 `LocalPlayer->GetProjectionData()` 실패(`:255`) — 뷰포트 크기가 0이 되는 창 최소화/알트탭이나 월드 전환 프레임에서 한 번만 발생해도 그 시점 등록돼 있던 모든 인디케이터가 다시 뜨지 않는다(대상이 파괴됐다 되살아나 플래그가 `false`를 한 번 거친 슬롯만 우연히 복구된다). Lyra `SActorCanvas`의 구조가 그대로 이식된 것으로 보인다.
- **제안**: `SetShowAnyIndicators`가 위젯 Visibility를 직접 만지지 말고 슬롯의 `SetIndicatorVisible(false)`를 호출해 플래그와 실제 Visibility를 한 곳에서만 동기화한다(또는 `true`로 되돌아올 때 전 슬롯 플래그를 강제로 dirty 처리).
- **확신도**: 높음

### 2. 🟡 어빌리티 VM이 이미 진행 중인 쿨다운을 반영하지 않고 초기화된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:22-57`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 2번 미해결) `Initialize`는 CDO에서 쿨다운 GE 클래스와 `StackLimitCount`만 읽고 `SetCurrentCharges(AbilityMaxRecharges)`(`:31`)로 항상 "충전 만땅·쿨다운 없음" 상태로 출발한다. 실제 쿨다운 표시는 `HandleGameplayEffectApplied`가 새 쿨다운 GE 적용을 받아야만 시작되므로(`:235-242`), VM이 쿨다운 도중에 생성되면(`UWxViewModel_AbilitySystem::GetOrCreateAbilityViewModel`은 UMG 바인딩 최초 평가 시 지연 생성한다 — 메뉴 최초 오픈, HUD 재생성 등 임의 시점) `IsOnCooldown=false`·`CooldownRemaining=0`·`CurrentCharges=Max`가 다음 발동까지 유지된다. 티커가 돌지 않으므로 자가 교정도 없고, `RefreshActivationState`가 `CanActivate=false`로 부분적으로만 가려준다.
- **제안**: `Initialize` 말미에서 `UpdateCooldownState(0.f)`를 1회 실행하고 반환값이 true면 티커를 등록한다(활성 쿨다운 GE 스캔 로직을 그대로 재사용).
- **확신도**: 높음

### 3. 🟡 어빌리티 발동 가능 재평가가 제네릭 태그 이벤트와 매 틱 GE 질의에 얹혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:42`, `:285`, `:324`, `:329-357`
- **범주**: 성능/안전
- **문제**: (직전 리뷰 4번 미해결) (a) VM마다 `RegisterGenericGameplayTagEvent()`를 구독해(`:42`) ASC의 **모든** 태그 변경 1건마다 `RefreshActivationState`를 돌린다. 이 함수는 `GetActivatableAbilities()` 선형 탐색 + `CanActivateAbility` + `CheckCost`(비용 GE 평가)를 수행하므로 비용이 `VM 수 × 태그 변경 수`로 곱해진다. (b) `UpdateCooldownState`는 쿨다운이 도는 동안 매 틱 `ASC->GetActiveEffects(Query)`(`:285`)로 `TArray`를 새로 할당받아 전 활성 GE를 훑고, 그 위에 다시 `RefreshActivationState`를 부른다(`:324`).
- **제안**: 태그 구독을 해당 어빌리티의 요건 태그(`ActivationRequiredTags`/`ActivationBlockedTags`) 단위 `RegisterGameplayTagEvent`로 좁히고, 매 틱 재평가는 dirty 플래그로 프레임당 1회로 합치거나 갱신 주기를 늘린다(UI 표시에 60Hz 정확도는 불필요).
- **확신도**: 중간(프로파일링 없이 코드 형태로만 판단 — 실제 VM 수·태그 트래픽에 좌우된다)

### 4. 🟡 네임플레이트가 숨김 상태에서도 매 틱 스케일을 갱신한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31-60` (기본 숨김은 `:18`, 틱 상시 활성은 `:12`)
- **범주**: 성능/안전
- **문제**: (직전 리뷰 5번 미해결) 표시 판정은 태그 이벤트 구동으로 정리됐지만 `TickComponent`는 가시성과 무관하게 매 프레임 `GetFirstPlayerController` → `GetPlayerViewPoint` → 거리 계산 → `SetRenderScale`을 수행한다. 네임플레이트 기본값이 숨김이라 월드의 적 대부분이 "보이지 않는 채로 매 프레임 렌더 트랜스폼을 갱신"하며, `UUserWidget::SetRenderScale`은 값이 같아도 위젯을 무효화하므로 적 수에 비례해 Slate 무효화가 쌓인다.
- **제안**: `RefreshVisibility`에서 판정 결과에 맞춰 `SetComponentTickEnabled()`를 함께 토글한다(표시 진실이 이미 한 곳에 모여 있어 추가 상태가 필요 없다). 계산된 `Scale`이 직전 값과 유의미하게 다를 때만 `SetRenderScale`을 부르는 것도 같이 적용할 수 있다.
- **확신도**: 높음

### 5. 🟡 이미지 스트리밍이 TMap 원소 참조를 가상 호출·브로드캐스트 너머로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:28`, `:53`, `:60-67`
- **범주**: 버그/정확성
- **문제**: `RequestImageAsync`는 `ImageRequests.FindOrAdd(FieldName)`로 얻은 참조를 유지한 채 `RequestAsyncLoad`를 호출하고 그 반환을 `Request.Handle`에 대입한다(`:53`). `HandleImageLoaded`도 맵 원소 포인터를 유지한 채 `ApplyLoadedImage`를 부르고 **그 뒤에** `Request->Handle.Reset()`을 한다(`:66-67`). `ApplyLoadedImage`는 파생 VM에서 `UE_MVVM_SET_PROPERTY_VALUE` → 필드 변경 브로드캐스트 → UMG/BP 재진입으로 이어질 수 있고, 재진입이 **다른 FieldName**으로 `RequestImageAsync`를 부르면 `FindOrAdd`가 맵을 재할당해 보관 중이던 참조/포인터가 댕글링된다. 클래스 주석이 다중 이미지 슬롯("서로 다른 필드끼리는 간섭하지 않는다", `WxViewModel.h:43`)을 공식 계약으로 내걸고 있어, 슬롯이 2개 이상인 VM이 생기는 순간 현실화되는 잠재 힙 손상이다(현재 파생 VM은 모두 슬롯 1개라 재해시가 일어나지 않는다).
- **제안**: `HandleImageLoaded`에서 `Handle.Reset()`을 `ApplyLoadedImage` **앞으로** 옮기고, `RequestImageAsync`도 핸들을 지역 변수로 받은 뒤 맵을 재조회해 대입한다.
- **확신도**: 중간

### 6. 🟡 폰이 교체되면 HUD가 레이어 스택에 중복으로 쌓인다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:264-274`
- **범주**: 설계/구조
- **문제**: `HandlePossessedPawnChanged`는 새 폰이 들어올 때마다 조건 없이 `PushSoftContentToLayer(UI_Layer_Game, GameHUDClass)`를 호출하는데(`:273`) 이전 HUD를 걷는 경로가 없다. `UCommonActivatableWidgetStack`은 새 위젯 push 시 기존 표시 위젯을 비활성화하기 **전에** `OnDeactivated` 구독을 먼저 끊으므로(엔진 `CommonActivatableWidgetContainer.cpp::SetSwitcherIndex`) 옛 HUD는 스택에서 제거되지 않고 남는다. 즉 재빙의 1회마다 HUD 인스턴스가 누적되고, `UWxHUDLayout::RebuildWidget`이 HUD마다 `SWxIndicatorCanvas`를 하나씩 만들므로(`WxHUDLayout.cpp:58-67`) 인디케이터 매니저를 구독하는 캔버스·액티브 타이머·위젯 풀도 함께 늘어난다. 현재 게임 코드에는 재빙의 경로가 없어(`AWxGameMode::HandleExperienceLoaded`는 `!GetPawn()`일 때만 `RestartPlayer`) 아직 드러나지 않은 잠재 결함이다.
- **제안**: HUD를 push하기 전에 이미 떠 있는 HUD가 있으면 걷거나, 서브시스템이 push한 HUD 인스턴스를 기억해 폰이 바뀌어도 재사용한다.
- **확신도**: 중간

### 7. 🟢 이펙트 VM 제거 경로가 자식 `Deinitialize()`를 건너뛴다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:169`, `:240`
- **범주**: 중복/복잡도
- **문제**: (직전 리뷰 3번 미해결) 같은 파일 `:38-39`가 "`Empty()`로 배열에서만 떼면 자식은 GC의 `BeginDestroy`→`Deinitialize`까지 티킹·구독을 유지한다"고 명시하고 `Deinitialize`에서 자식마다 명시적으로 정리하는데, `RefreshActiveEffectViewModels`의 `Empty()`(`:169`)와 `HandleActiveEffectRemoved`의 `RemoveAt`(`:240`)은 그 규약을 따르지 않는다. 실질 피해는 작다 — `RefreshActiveEffectViewModels`는 `Deinitialize` 직후에만 호출돼 배열이 이미 비어 있고, 버려진 이펙트 VM의 티커는 다음 틱에 `GetActiveGameplayEffect`가 null을 돌려주며 스스로 멈춘다. 규약이 한 파일 안에서 갈리는 것이 문제다.
- **제안**: 두 지점 모두 배열에서 떼기 전에 대상 VM의 `Deinitialize()`를 호출하도록 통일한다.
- **확신도**: 중간

### 8. 🟢 `InitializeViewModels`가 ASC 널을 검증하지 않고 재호출도 방어하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:72-98`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 10번 미해결) 위젯과 `UMVVMView` 부재는 각각 조기 반환으로 막으면서(`:74-84`) `InASC`는 검사 없이 `:94`에서 `RegisterGenericGameplayTagEvent()`로 역참조한다. 또 두 번 호출하면 같은 델리게이트에 중복 바인딩되고 이전 `UWxViewModel_Character`는 `Deinitialize()` 없이 버려진다. 현 유일 호출부(`Source/WxGame/Character/WxEnemyCharacter.cpp:44`)가 유효한 ASC를 1회만 넘겨 발현하지 않을 뿐, 플러그인 공개 API라 소비 측이 늘면 노출된다.
- **제안**: 선두에 `if (!InASC) { return; }`를 추가하고, 재초기화 시 기존 구독을 `RemoveAll(this)`로 떼고 이전 VM에 `Deinitialize()`를 호출한다.
- **확신도**: 높음

### 9. 🟢 `HandleTabCreation_Implementation`의 `TabButton` 널 검사 순서가 뒤집혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:122`, `:131`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 8번 미해결) `:122`가 `TabButton->GetClass()`를 검사 없이 역참조하는데 `:131`에서는 같은 포인터를 `if (TabButtonContainer && TabButton)`으로 방어한다. 같은 함수 안에서 같은 포인터에 대한 가정이 상충하며, 방어가 필요한 전제라면 `:122`가 먼저 터진다(엔진 `UCommonTabListWidgetBase`는 유효한 버튼만 넘기므로 실제 발생 가능성은 낮다).
- **제안**: 함수 진입부에서 `if (!TabButton) { return; }`으로 한 번에 정리하고 `:131`의 중복 검사를 제거한다.
- **확신도**: 높음(일관성 결함은 확정)

### 10. 🟢 쿨다운 퍼센트만 클램프되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:320`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 9번 미해결) `SetCooldownPercent(NextChargeRemaining / CooldownDuration)`에 상한이 없다. `CooldownDuration`은 첫 적용 시에만 기록되고(`:230-233`) 충전이 모두 회복될 때까지 갱신되지 않으므로, 도중에 더 긴 쿨다운이 적용되면 1을 초과한 값이 프로그레스 바로 흘러간다. 동일 패턴인 `UWxViewModel_Effect::UpdateEffectState`는 `FMath::Min(..., 1.f)`로 막는다(`WxViewModel_Effect.cpp:185`).
- **제안**: Effect VM과 동일하게 `FMath::Clamp(..., 0.f, 1.f)`를 적용한다.
- **확신도**: 중간

### 11. 🟢 `EffectEndTime`은 계산만 하고 아무도 읽지 않는 데드 필드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h:91` · `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:53`
- **범주**: 중복/복잡도
- **문제**: (직전 리뷰 11번 미해결) `Initialize`에서 `EffectEndTime = CurrentTime + Remaining`을 기록하지만 `UpdateEffectState`는 `ActiveEffect->StartWorldTime + CachedDuration`으로 매번 다시 계산하며(`:183`) 이 필드를 읽지 않는다. `Deinitialize`도 `EffectEndTime`/`CachedDuration`을 되돌리지 않아 "남은 값"의 의미가 모호하다.
- **제안**: 필드를 제거하거나, 남긴다면 `UpdateEffectState`가 이 값을 진실로 사용하도록 통일하고 `Deinitialize`에서 초기화한다.
- **확신도**: 높음

### 12. 🟢 규칙 위반 — override 2곳이 `Super::`를 호출하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:22` (`Activate`) · `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:108` (`HandleTabCreation_Implementation`)
- **범주**: 규칙 위반
- **문제**: (직전 리뷰 7번 미해결) 두 베이스 구현이 현재 비어 있어 동작 차이는 없지만, 값 전체를 대체하는 `GetDesiredInputConfig`(이전 리뷰에서 예외로 판정)와 달리 여기서는 `Super::` 호출이 무해하며 엔진 상위 구현이 나중에 채워질 경우를 대비한다.
- **제안**: 각 함수 선두에 `Super::Activate();` / `Super::HandleTabCreation_Implementation(TabId, TabButton);`을 추가한다.
- **확신도**: 높음

### 13. 🟢 규칙 위반 — 위젯/뷰모델의 `BlueprintCallable` 사용 범위
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:71`, `:78`, `:81`, `:84`, `:87`, `:90`, `:93` · `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:49`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5는 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action 팩토리로 제한하며, 프로젝트가 인정한 예외는 위젯 서브클래스의 MVVM 수신용 1-arg setter뿐이다(`WxButtonBase.h:23`이 이에 해당해 제외). `UWxTabListWidgetBase`의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)는 Lyra `ULyraTabListWidgetBase` 이식 시 원본 지정자가 그대로 넘어온 것으로 보이며, 그중 4개는 `BlueprintCallable, BlueprintPure`를 함께 붙여 지정자 자체가 중복이다(`BlueprintPure`가 호출 가능을 함의). `UWxViewModel_Ability::TryActivateAbility`는 주석대로 위젯 `OnClicked`의 MVVM 이벤트 바인딩 대상이라 UFUNCTION 노출이 사실상 필수인 정당한 예외로 보인다.
- **제안**: TabList의 순수 조회 6개는 `BlueprintPure`만 남기고, 실제 변경 함수 2개(`SetTabHiddenState`·`RegisterDynamicTab`)는 BP 노출이 필요한지 재검토해 필요하면 `UWxUILibrary` 파사드로 옮긴다. `TryActivateAbility`는 유지하되 "MVVM 이벤트 바인딩 대상 함수"를 CLAUDE.md의 명시 예외로 추가해 규칙과 코드의 간극을 없앤다.
- **확신도**: 중간(직전 리뷰는 이를 "의도된 관용 패턴"으로 판정해 재보고하지 않았다 — 규칙 문면상으로는 위반이므로 예외 명문화 여부를 한 번 결정하는 편이 낫다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/SWxIndicatorCanvas.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/SWxIndicatorCanvas.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Selection.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, 및 대응 `Public/` 헤더 전반
- **확인 후 위반 0건**: 모듈 경계(`.uplugin`·`Build.cs`·include 모두 Wx 중 `WxCore`만 참조 — `WxUI.Build.cs:27`), 전 소스 첫 줄 Copyright 주석, `Wx` prefix, `FORCEINLINE`/인라인 함수 정의, 델리게이트 콜백의 `Handle` prefix
- **미검토 / 한계**:
  - `PrimaryGameLayout`이 단일 필드라 로컬 플레이어가 둘 이상이면 마지막 플레이어의 레이아웃만 남는다(`WxUIManagerSubsystem.cpp:243-247`). 코드·README·인디케이터 노드 주석이 모두 "v1 싱글/리슨 호스트 전제"를 명시하고 있어 의도된 제약으로 보아 발견으로 올리지 않았다 — 스플릿스크린 계획이 생기면 `TMap<ULocalPlayer*, ...>` 승격이 필요하다.
  - `FWxStateTreeTask_MarkIndicator::Tick`이 매 틱 `FUniversalObjectLocator::SyncFind`로 대상을 재해석하는 비용(`WxIndicatorStateTreeNodes.cpp:41-63`)은 월드 파티션 언로드/재로드 추종이라는 의도가 주석에 명시돼 있고 실측이 없어 제외했다. `SWxIndicatorCanvas::OnArrangeChildren`의 페인트마다 발생하는 `SortedSlots` 임시 배열 할당(`:54-59`)도 같은 이유로 제외했다.
  - `UWxActionWidget`/`UWxButtonBase`의 에디터 전용 아이콘 해석 경로는 디자인타임 한정이라 얕게만 봤다. BP/WBP 내부(위젯 계층·MVVM 바인딩 그래프·디폴트값)와 `Plugins/WxUI/Content/`는 범위 밖이며, CommonUI 스택의 위젯 풀링·전이 타이밍은 엔진 소스(UE 5.8)로 대조해 확인했다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 60파일 — `/module-review`로 갱신*
