# WxUI — 코드 리뷰

> 수명·구독 해제 규약이 주석으로 명문화된 건강한 모듈이다. 직전 리뷰(2026-07-31)의 최대 결함이던 인디케이터 Slate 캔버스는 뷰모델 방식으로 통째 교체되며 사라졌고, 어빌리티 VM의 쿨다운 시드 누락(`SeedActiveCooldown` 추가)과 네임플레이트의 숨김 상태 매 틱 갱신도 해소됐다. 이번 리뷰는 `UWxUIManagerSubsystem`·VM 계층 전체·인디케이터 매니저/ST 노드·네임플레이트를 cpp까지 깊게 보고, 위젯 베이스·설정·모듈 파일은 훑었다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 어트리뷰트 VM 캐시 키에 Max 어트리뷰트가 빠져 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:116` (조회 구현은 같은 파일 `:71`)
- **범주**: 버그/정확성
- **문제**: `GetOrCreateAttributeViewModel(Current, Max)`는 캐시를 `FindAttributeViewModel(Current)`로만 뒤진다. 키가 Current 하나뿐이라 Max가 다른 요청도 같은 VM을 돌려받는다. 어떤 위젯이 `(Health, MaxHealth)`로 먼저 VM을 만들면, 뒤에 `(Health, 무효)`로 요청한 바인딩은 "Current를 최대값으로 쓰는 VM"(`:123`의 폴백) 대신 MaxHealth 기반 VM을 받고 그 반대도 성립한다. 결과적으로 `AttributePercent`·`IsAttributeFull`이 요청자가 기대한 것과 다른 값을 낸다. 어느 위젯이 먼저 평가되느냐에 따라 갈리므로 재현이 불규칙하다. 어빌리티 쪽(`:139`)도 `HasAll` 술어로 캐시를 뒤져, 넓은 태그 요청이 먼저 만들어진 좁은 어빌리티의 VM에 걸릴 수 있는 같은 성질의 순서 의존을 갖는다.
- **제안**: 캐시 키를 `(Current, Max)` 쌍으로 확장한다(`FindAttributeViewModel`에 Max 인자 추가). 어빌리티는 VM에 "생성을 유발한 요청 태그"를 함께 보관해 그 값으로 조회하면 순서 의존이 사라진다.
- **확신도**: 높음(키 누락은 확정 — 실제 오작동 여부는 WBP 바인딩 구성에 달림)

### 2. 🟡 이미지 스트리밍이 TMap 원소 참조를 가상 호출·브로드캐스트 너머로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:28`, `:53`, `:64`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 5번 미해결) `RequestImageAsync`는 `ImageRequests.FindOrAdd(FieldName)`로 얻은 참조를 든 채 `RequestAsyncLoad`를 부르고 그 반환을 `Request.Handle`에 대입한다(`:53`). `HandleImageLoaded`도 맵 원소 포인터를 유지한 채 `ApplyLoadedImage`를 부르고 **그 뒤에** `Request->Handle.Reset()`을 한다(`:70-71`). `ApplyLoadedImage`는 파생 VM에서 필드 변경 브로드캐스트 → UMG 재진입으로 이어질 수 있고, 재진입이 **다른 FieldName**으로 `RequestImageAsync`를 부르면 `FindOrAdd`가 재해시하며 보관 중이던 참조/포인터가 댕글링된다. 베이스가 다중 슬롯을 공식 계약으로 내걸고 있어(`WxViewModel.h:43` "서로 다른 필드끼리는 간섭하지 않는다") 슬롯 2개짜리 VM이 생기는 순간 현실화되는 잠재 힙 손상이다. 현재 파생 VM은 모두 슬롯 1개라 재해시가 일어나지 않는다.
- **제안**: `HandleImageLoaded`에서 `Handle.Reset()`을 `ApplyLoadedImage` **앞으로** 옮기고(로드 결과는 지역 변수로 먼저 꺼낸다), `RequestImageAsync`도 핸들을 지역 변수로 받은 뒤 맵을 재조회해 대입한다.
- **확신도**: 중간(현재 코드 경로에선 미발현)

### 3. 🟡 폰이 교체되면 HUD가 Game 레이어에 중복으로 쌓인다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269` (push는 `:278`)
- **범주**: 설계/구조
- **문제**: (직전 리뷰 6번 미해결) `HandlePossessedPawnChanged`는 새 폰이 들어올 때마다 조건 없이 `PushSoftContentToLayer(UI.Layer.Game, GameHUDClass)`를 호출하는데, 이전 HUD를 걷는 경로가 코드 어디에도 없다(레이어를 비우는 것은 BP 파사드 `DeactivateWidgetsInLayer`뿐). 레이아웃 전체 재생성은 PC가 바뀔 때만 일어나므로(`:248`) 같은 PC가 폰만 갈아타는 흐름(탈것·연출용 폰·서버 재빙의)에서는 정리되지 않고, 재빙의 1회마다 HUD 인스턴스가 누적된다. 밑에 깔린 HUD는 비활성 상태로 살아남아 그 안의 뷰모델 구독도 함께 남는다. 현재 게임 코드에 재빙의 경로가 없어 아직 드러나지 않은 잠재 결함이다.
- **제안**: push 전에 이미 떠 있는 HUD를 걷거나(대화 창이 `DialogueScreen`을 다루는 방식과 동일), 서브시스템이 push한 HUD 인스턴스를 기억해 폰이 바뀌어도 재사용한다.
- **확신도**: 중간

### 4. 🟡 규칙 위반 — 위젯 클래스의 `BlueprintCallable` 사용 범위
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`, `:73`, `:76`, `:79`, `:82`, `:85`, `:88`
- **범주**: 규칙 위반
- **문제**: (직전 리뷰 13번 미해결) `CLAUDE.md` 코딩 규칙 5는 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action 팩토리로 제한한다. `UWxTabListWidgetBase`의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)가 범위 밖이며, 그중 4개는 `BlueprintCallable, BlueprintPure`를 함께 붙여 지정자 자체가 중복이다(`BlueprintPure`가 호출 가능을 함의). Lyra `ULyraTabListWidgetBase` 이식 시 원본 지정자가 그대로 넘어온 것으로 보인다. (`UWxViewModel_Ability::TryActivateAbility`는 뷰모델 Command 예외, `UWxButtonBase::SetButtonText`는 위젯의 1-arg setter 관용 예외로 판정해 제외했다.)
- **제안**: 순수 조회 5개는 `BlueprintPure`만 남기고, 변경 함수 2개(`SetTabHiddenState`·`RegisterDynamicTab`)는 BP 노출이 실제로 필요한지 확인해 필요하면 `UWxUILibrary` 파사드로 옮긴다.
- **확신도**: 높음(규칙 문면상 위반은 확정 — 예외로 명문화할지 한 번 결정할 필요는 있다)

### 5. 🟡 어빌리티 발동 가능 재평가가 제네릭 태그 이벤트와 매 틱 GE 질의에 얹혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:42`, `:335`, `:374`
- **범주**: 성능/안전
- **문제**: (직전 리뷰 3번 미해결) (a) VM마다 `RegisterGenericGameplayTagEvent()`를 구독해(`:42`) ASC의 **모든** 태그 변경 1건마다 `RefreshActivationState`를 돌린다. 이 함수는 `GetActivatableAbilities()` 선형 탐색 + `CanActivateAbility` + `CheckCost`를 수행하므로 비용이 `VM 수 × 태그 변경 수`로 곱해진다. (b) 쿨다운이 도는 동안에는 매 틱 `ASC->GetActiveEffects(Query)`(`:335`)가 `TArray`를 새로 할당해 전 활성 GE를 훑고, 그 위에 다시 `RefreshActivationState`를 부른다(`:374`) — `CanActivateAbility` 내부가 쿨다운·비용을 또 조회하므로 GE 순회가 프레임마다 두 번 겹친다. HUD 슬롯 6칸이 동시에 쿨다운이면 그대로 6배다.
- **제안**: 태그 구독을 해당 어빌리티의 요건 태그(`ActivationRequiredTags`/`ActivationBlockedTags`) 단위 `RegisterGameplayTagEvent`로 좁힌다. 매 틱 재평가는 이미 알고 있는 "가장 이른 만료 시각"에 맞춰 예약하거나 갱신 주기를 0.1초로 낮춘다 — 표시용이라 한두 프레임 지연은 문제되지 않는다.
- **확신도**: 중간(프로파일링 없이 코드 형태로만 판단)

### 6. 🟡 `Mark Indicator` 태스크가 이미 등록된 상태에서도 매 틱 UOL을 다시 해석한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:41` (호출은 `:119`)
- **범주**: 성능/안전
- **문제**: `Tick` → `RefreshIndicator` → `ResolveTargetActor` 순으로 매 프레임 `FUniversalObjectLocator::SyncFind`가 돈다. "이미 등록돼 있는가" 검사(`:51`)는 해석이 끝난 **뒤에** 하므로, 정상 표시 중에도 경로 해석 비용을 계속 낸다. 월드 파티션 스트리밍을 그대로 따라가려는 의도는 주석에 명시돼 있지만, 등록 이후에는 등록증이 대상 컴포넌트를 이미 들고 있어 해석 결과가 바뀌는 경우가 드물다.
- **제안**: 등록증이 유효하고 그 대상 컴포넌트도 유효하면 해석 자체를 건너뛰고, 무효해졌을 때만 다시 해석한다. 재시도 간격을 0.2~0.5초로 낮춰도 스트리밍 추종에는 문제가 없다.
- **확신도**: 중간(의도된 트레이드오프일 수 있음 — 다만 "검사 후 해석" 순서 교체는 의미 변화 없이 비용만 줄인다)

### 7. 🟢 이펙트 VM 제거 경로가 자식 `Deinitialize()`를 건너뛴다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:169`, `:240`
- **범주**: 중복/복잡도
- **문제**: (직전 리뷰 7번 미해결) 같은 파일 `:38`이 "`Empty()`로 배열에서만 떼면 자식은 GC의 `BeginDestroy`→`Deinitialize`까지 티킹·구독을 유지한다"고 명시하고 `Deinitialize`에서 자식마다 명시적으로 정리하는데, `RefreshActiveEffectViewModels`의 `Empty()`(`:169`)와 `HandleActiveEffectRemoved`의 `RemoveAt`(`:240`)은 그 규약을 따르지 않는다. 실질 피해는 작다 — 전자는 `Deinitialize` 직후에만 호출돼 배열이 이미 비어 있고, 버려진 이펙트 VM의 티커는 다음 틱에 `GetActiveGameplayEffect`가 null을 돌려주며 스스로 멈춘다. 규약이 한 파일 안에서 갈리는 것이 문제다.
- **제안**: 두 지점 모두 배열에서 떼기 전에 대상 VM의 `Deinitialize()`를 호출하도록 통일한다.
- **확신도**: 중간

### 8. 🟢 쿨다운 퍼센트만 클램프되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:370`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 9번 미해결) `SetCooldownPercent(NextChargeRemaining / CooldownDuration)`에 상한이 없다. `CooldownDuration`은 첫 적용 시점에만 기록되고(`:280-283`, 시드는 `:88`) 충전이 모두 회복될 때까지 갱신되지 않으므로, 도중에 더 긴 쿨다운이 적용되면 1을 초과한 값이 프로그레스 바로 흘러간다. 동일 패턴인 `UWxViewModel_Effect::UpdateEffectState`는 `FMath::Min(..., 1.f)`로 막는다(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:185`).
- **제안**: Effect VM과 동일하게 `FMath::Clamp(..., 0.f, 1.f)`를 적용한다.
- **확신도**: 중간

### 9. 🟢 `InitializeViewModels`가 ASC 널을 검증하지 않고 재호출도 방어하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:110` (함수는 `:88`)
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 10번 미해결) 위젯과 `UMVVMView` 부재는 각각 조기 반환으로 막으면서(`:90-100`) `InASC`는 검사 없이 `:110`에서 `RegisterGenericGameplayTagEvent()`로 역참조한다 — null이 들어오면 여기서 죽는다(자식 VM의 `Initialize`는 null을 조용히 걸러내므로 절반만 방어된 상태다). 두 번 호출하면 같은 델리게이트에 중복 바인딩되고 이전 `UWxViewModel_Character`는 `Deinitialize()` 없이 버려진다. 현 유일 호출부(`Source/WxGame/Character/WxEnemyCharacter.cpp:44`)가 기본 서브오브젝트를 1회만 넘겨 발현하지 않을 뿐, 플러그인 공개 API라 소비 측이 늘면 노출된다.
- **제안**: 선두에 `if (!InASC) { return; }`을 추가하고, 재초기화 시 기존 구독을 `RemoveAll(this)`로 떼고 이전 VM에 `Deinitialize()`를 호출한다(`EndPlay`가 이미 같은 정리를 한다).
- **확신도**: 높음(코드 사실), 실제 크래시 가능성은 낮음

### 10. 🟢 `HandleTabCreation_Implementation`의 `TabButton` 널 검사 순서가 뒤집혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:127`, `:136`
- **범주**: 버그/정확성
- **문제**: (직전 리뷰 8번 미해결) `:127`이 `TabButton->GetClass()`를 검사 없이 역참조하는데 `:136`에서는 같은 포인터를 `if (TabButtonContainer && TabButton)`으로 방어한다. 같은 함수 안에서 같은 포인터에 대한 가정이 상충하며, 방어가 필요한 전제라면 `:127`이 먼저 터진다(엔진 `UCommonTabListWidgetBase`는 유효한 버튼만 넘기므로 실제 발생 가능성은 낮다).
- **제안**: 함수 진입부에서 `if (!TabButton) { return; }`으로 한 번에 정리하고 `:136`의 중복 검사를 제거한다.
- **확신도**: 높음(일관성 결함은 확정)

### 11. 🟢 `EffectEndTime`은 계산만 하고 아무도 읽지 않는 데드 필드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h:91` · `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:53`
- **범주**: 중복/복잡도
- **문제**: (직전 리뷰 11번 미해결) `Initialize`에서 `EffectEndTime = CurrentTime + Remaining`을 기록하지만 `UpdateEffectState`는 `ActiveEffect->StartWorldTime + CachedDuration`으로 매번 다시 계산하며(`:183`) 이 필드를 읽지 않는다. `Deinitialize`도 `EffectEndTime`/`CachedDuration`을 되돌리지 않아 "남은 값"의 의미가 모호하다.
- **제안**: 필드와 대입을 함께 제거한다.
- **확신도**: 높음

### 12. 🟢 자유 커서 해제가 HUD의 입력 설정을 하드코딩해 이중 정의가 됐다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:76` (원본은 `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp:12`)
- **범주**: 중복/복잡도
- **문제**: `HandleFreeCursorReleased`가 `FUIInputConfig(Game, CapturePermanently)`를 직접 만들어 복원한다. 같은 값이 `UWxActivatableWidget::GetDesiredInputConfig`에도 있어, HUD WBP가 `InputMode`를 Menu로 바꾸면 복원 경로만 옛 설정으로 되돌아간다(눌렀다 뗀 뒤에야 입력 모드가 어긋나는 형태라 추적이 어렵다).
- **제안**: 복원 시 `GetDesiredInputConfig()` 결과를 그대로 쓴다(값이 없으면 아무 것도 하지 않음).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Selection.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxSubtitleStateTreeNodes.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, 및 대응 `Public/` 헤더 전반
- **확인 후 위반 0건**: 모듈 경계(`.uplugin`·`Build.cs`·include 모두 Wx 중 `WxCore`만 참조 — `WxUI.Build.cs:27`, 역방향 의존은 `WxGame`/`WxEditor`뿐) · 전 소스 63개 첫 줄 Copyright 주석 · `Wx` prefix · `FORCEINLINE`/헤더 인라인 정의(StateTree `GetInstanceDataType` 2건은 파일 주석에 예외 근거 명시, `PushWidgetToLayerStack`은 템플릿이라 헤더 정의가 불가피) · 델리게이트 콜백의 `Handle` prefix · 람다 2건(`WxUILibrary.cpp:25` 다이나믹→네이티브 델리게이트 브릿지, `WxTabListWidgetBase.cpp:44` `FindByPredicate`)은 모두 API상 필요한 자리
- **직전 리뷰 대비 해소 확인**: 인디케이터 Slate 캔버스 결함(전체 숨김 후 복구 불가)은 캔버스 자체가 뷰모델 방식으로 교체되며 소멸 · 어빌리티 VM의 진행 중 쿨다운 미반영은 `SeedActiveCooldown`(`WxViewModel_Ability.cpp:62`)으로 해결 · 네임플레이트의 숨김 상태 매 틱 갱신은 `IsVisible()` 게이트와 `LastRenderScale` 비교로 해결(`WxNameplateComponent.cpp:38`, `:69`)
- **미검토 / 한계**:
  - `PrimaryGameLayout`·`TrackedPlayerController`가 단일 필드라 로컬 플레이어가 둘 이상이면 나중 플레이어가 앞선 플레이어의 레이아웃을 철거한다(`WxUIManagerSubsystem.cpp:248`). 코드·README·ST 노드 주석이 모두 "v1 싱글/리슨 호스트 전제"를 명시하므로 의도된 제약으로 보아 발견으로 올리지 않았다 — 스플릿스크린 계획이 생기면 `TMap<ULocalPlayer*, ...>` 승격이 필요하다.
  - `UWxViewModel_Subtitle`이 글로벌 컬렉션에 등록된 뒤 해제되지 않아 월드가 바뀌어도 문구가 남을 수 있는지는 실행 확인이 필요해 판단을 보류했다(`WxViewModel_Subtitle.cpp:42`).
  - `UWxActionWidget::GetIcon`의 에디터 전용 IMC 전수 스캔은 디자인타임 한정이라 런타임 관점에서 깊게 보지 않았다. BP/WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)와 `UWxUIDeveloperSettings`의 실제 config 값은 범위 밖이다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 63파일 — `/module-review`로 갱신*
