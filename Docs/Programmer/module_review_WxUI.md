# WxUI — 코드 리뷰

> 수명·구독 해제 규약이 주석으로 명문화된 건강한 모듈이다. 직전 리뷰(2026-08-03) 이후 자막이 데이터테이블 기반으로 재작성됐고 탭 버튼 널 검사·`EffectEndTime` 데드 필드는 해소됐으나, 뷰모델 캐시 키·이미지 스트리밍 재진입·HUD 중복 push 등 구조적 지적은 그대로 남아 있다. 이번 리뷰는 `UWxUIManagerSubsystem`·VM 계층 전체·인디케이터 매니저/ST 노드·신규 자막 ST 노드를 cpp까지 깊게 보고, 위젯 베이스·설정·모듈 파일은 훑었다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 어트리뷰트 VM 캐시 키에 Max 어트리뷰트가 빠져 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:116` (조회 구현은 같은 파일 `:71`)
- **범주**: 버그/정확성
- **문제**: `GetOrCreateAttributeViewModel(Current, Max)`는 캐시를 `FindAttributeViewModel(Current)`로만 뒤진다. 키가 Current 하나뿐이라 Max가 다른 요청도 같은 VM을 돌려받는다. 어떤 위젯이 `(Health, MaxHealth)`로 먼저 VM을 만들면, 뒤에 `(Health, 무효)`로 요청한 바인딩은 "Current를 최대값으로 쓰는 VM"(`:123`의 폴백) 대신 MaxHealth 기반 VM을 받고 그 반대도 성립한다. 결과적으로 `AttributePercent`·`IsAttributeFull`이 요청자가 기대한 것과 다른 값을 낸다. 어느 위젯이 먼저 평가되느냐에 따라 갈리므로 재현이 불규칙하다. 어빌리티 쪽(`:139`)도 `HasAll` 술어로 캐시를 뒤져, 넓은 태그 요청이 먼저 만들어진 좁은 어빌리티의 VM에 걸릴 수 있는 같은 성질의 순서 의존을 갖는다.
- **제안**: 캐시 키를 `(Current, Max)` 쌍으로 확장한다(`FindAttributeViewModel`에 Max 인자 추가). 어빌리티는 VM에 "생성을 유발한 요청 태그"를 함께 보관해 그 값으로 조회하면 순서 의존이 사라진다.
- **확신도**: 높음(키 누락은 확정 — 실제 오작동 여부는 WBP 바인딩 구성에 달림)

### 2. 🟡 이미지 스트리밍이 TMap 원소 참조를 가상 호출 너머로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:28`, `:53`, `:64`, `:70-71`
- **범주**: 버그/정확성
- **문제**: `RequestImageAsync`는 `ImageRequests.FindOrAdd(FieldName)`로 얻은 참조를 든 채 `RequestAsyncLoad`를 부르고 그 반환을 `Request.Handle`에 대입한다(`:53`). `HandleImageLoaded`도 맵 원소 포인터를 유지한 채 `ApplyLoadedImage`를 부르고 **그 뒤에** `Request->Handle.Reset()`을 한다(`:70-71`). `ApplyLoadedImage`는 파생 VM에서 필드 변경 브로드캐스트 → UMG 재진입으로 이어질 수 있고, 재진입이 **다른 FieldName**으로 `RequestImageAsync`를 부르면 `FindOrAdd`가 재해시하며 보관 중이던 참조/포인터가 댕글링된다. 베이스가 다중 슬롯을 공식 계약으로 내걸고 있어(`WxViewModel.h:43` "서로 다른 필드끼리는 간섭하지 않는다") 슬롯 2개짜리 VM이 생기는 순간 현실화되는 잠재 힙 손상이다. 현재 파생 VM은 모두 슬롯 1개라 재해시가 일어나지 않는다.
- **제안**: `HandleImageLoaded`에서 `Handle.Reset()`을 `ApplyLoadedImage` **앞으로** 옮기고(로드 결과는 지역 변수로 먼저 꺼낸다), `RequestImageAsync`도 핸들을 지역 변수로 받은 뒤 맵을 재조회해 대입한다.
- **확신도**: 중간(현재 코드 경로에선 미발현)

### 3. 🟡 폰이 교체되면 HUD가 Game 레이어에 중복으로 쌓인다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269` (push는 `:278`)
- **범주**: 설계/구조
- **문제**: `HandlePossessedPawnChanged`는 새 폰이 들어올 때마다 조건 없이 `PushSoftContentToLayer(UI.Layer.Game, GameHUDClass)`를 호출하는데, 이전 HUD를 걷는 경로가 코드 어디에도 없다(레이어를 비우는 것은 BP 파사드 `DeactivateWidgetsInLayer`뿐). 레이아웃 전체 재생성은 PC가 바뀔 때만 일어나므로(`:248`) 같은 PC가 폰만 갈아타는 흐름(탈것·연출용 폰·클라이언트의 `OnRep_Pawn` 재설정)에서는 정리되지 않고, 재빙의 1회마다 HUD 인스턴스가 누적된다. 밑에 깔린 HUD는 비활성 상태로 살아남아 그 안의 뷰모델 구독도 함께 남는다. 대화 창은 `DialogueScreen`으로 push한 인스턴스를 기억해 같은 문제를 이미 피하고 있다(`:331`, `:334`).
- **제안**: push 전에 이미 떠 있는 HUD를 걷거나, 서브시스템이 push한 HUD 인스턴스를 기억해 폰이 바뀌어도 재사용한다.
- **확신도**: 중간

### 4. 🟡 규칙 위반 — 위젯 클래스의 `BlueprintCallable` 사용 범위
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`, `:73`, `:76`, `:79`, `:82`, `:85`, `:88`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5는 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action 팩토리로 제한한다. `UWxTabListWidgetBase`의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)가 범위 밖이며, 그중 4개는 `BlueprintCallable, BlueprintPure`를 함께 붙여 지정자 자체가 중복이다(`BlueprintPure`가 호출 가능을 함의). Lyra `ULyraTabListWidgetBase` 이식 시 원본 지정자가 그대로 넘어온 것으로 보인다. (`UWxViewModel_Ability::TryActivateAbility`는 뷰모델 Command 예외, `UWxButtonBase::SetButtonText`는 위젯의 1-arg setter 관용 예외로 판정해 제외했다.)
- **제안**: 순수 조회 5개는 `BlueprintPure`만 남기고, 변경 함수 2개(`SetTabHiddenState`·`RegisterDynamicTab`)는 BP 노출이 실제로 필요한지 확인해 필요하면 `UWxUILibrary` 파사드로 옮긴다.
- **확신도**: 높음(규칙 문면상 위반은 확정 — 예외로 명문화할지 한 번 결정할 필요는 있다)

### 5. 🟡 어빌리티 발동 가능 재평가가 제네릭 태그 이벤트와 매 틱 GE 질의에 얹혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:42`, `:335`, `:374`
- **범주**: 성능/안전
- **문제**: (a) VM마다 `RegisterGenericGameplayTagEvent()`를 구독해(`:42`) ASC의 **모든** 태그 변경 1건마다 `RefreshActivationState`를 돌린다. 이 함수는 `GetActivatableAbilities()` 선형 탐색 + `CanActivateAbility` + `CheckCost`를 수행하므로 비용이 `VM 수 × 태그 변경 수`로 곱해진다. (b) 쿨다운이 도는 동안에는 매 틱 `ASC->GetActiveEffects(Query)`(`:335`)가 `TArray`를 새로 할당해 전 활성 GE를 훑고, 그 위에 다시 `RefreshActivationState`를 부른다(`:374`) — `CanActivateAbility` 내부가 쿨다운·비용을 또 조회하므로 GE 순회가 프레임마다 두 번 겹친다. HUD 슬롯 6칸이 동시에 쿨다운이면 그대로 6배다.
- **제안**: 태그 구독을 해당 어빌리티의 요건 태그(`ActivationRequiredTags`/`ActivationBlockedTags`) 단위 `RegisterGameplayTagEvent`로 좁힌다. 매 틱 재평가는 이미 알고 있는 "가장 이른 만료 시각"에 맞춰 예약하거나 갱신 주기를 0.1초로 낮춘다 — 표시용이라 한두 프레임 지연은 문제되지 않는다.
- **확신도**: 중간(프로파일링 없이 코드 형태로만 판단)

### 6. 🟡 `Mark Indicator` 태스크가 이미 등록된 상태에서도 매 틱 UOL을 다시 해석한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:44` (검사는 `:51`, 호출은 `:122`)
- **범주**: 성능/안전
- **문제**: `Tick` → `RefreshIndicator` → `ResolveTargetActor` 순으로 매 프레임 `FUniversalObjectLocator::SyncFind`가 돈다. "이미 등록돼 있는가" 검사(`:51`)는 해석이 끝난 **뒤에** 하므로, 정상 표시 중에도 경로 해석 비용을 계속 낸다. 월드 파티션 스트리밍을 그대로 따라가려는 의도는 주석에 명시돼 있지만, 등록 이후에는 등록증이 대상 컴포넌트를 이미 들고 있어(`WxIndicatorDescriptor.h:52`) 해석 결과가 바뀌는 경우가 드물다.
- **제안**: 등록증이 유효하고 그 대상 컴포넌트도 유효하면 해석 자체를 건너뛰고, 무효해졌을 때만 다시 해석한다. 재시도 간격을 0.2~0.5초로 낮춰도 스트리밍 추종에는 문제가 없다.
- **확신도**: 중간(의도된 트레이드오프일 수 있음 — 다만 "검사 후 해석" 순서 교체는 의미 변화 없이 비용만 줄인다)

### 7. 🟢 이펙트 VM 제거 경로가 자식 `Deinitialize()`를 건너뛴다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:169`, `:240`
- **범주**: 중복/복잡도
- **문제**: 같은 파일 `:38`이 "`Empty()`로 배열에서만 떼면 자식은 GC의 `BeginDestroy`→`Deinitialize`까지 티킹·구독을 유지한다"고 명시하고 `Deinitialize`에서 자식마다 명시적으로 정리하는데, `RefreshActiveEffectViewModels`의 `Empty()`(`:169`)와 `HandleActiveEffectRemoved`의 `RemoveAt`(`:240`)은 그 규약을 따르지 않는다. 실질 피해는 작다 — 전자는 `Deinitialize` 직후에만 호출돼 배열이 이미 비어 있고, 버려진 이펙트 VM의 티커는 다음 틱에 `GetActiveGameplayEffect`가 null을 돌려주며 스스로 멈춘다. 규약이 한 파일 안에서 갈리는 것이 문제다.
- **제안**: 두 지점 모두 배열에서 떼기 전에 대상 VM의 `Deinitialize()`를 호출하도록 통일한다.
- **확신도**: 중간

### 8. 🟢 쿨다운 퍼센트만 클램프되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:370`
- **범주**: 버그/정확성
- **문제**: `SetCooldownPercent(NextChargeRemaining / CooldownDuration)`에 상한이 없다. `CooldownDuration`은 첫 적용 시점에만 기록되고(`:280-283`, 시드는 `:88`) 충전이 모두 회복될 때까지 갱신되지 않으므로, 도중에 더 긴 쿨다운이 적용되면 1을 초과한 값이 프로그레스 바로 흘러간다. 동일 패턴인 `UWxViewModel_Effect::UpdateEffectState`는 `FMath::Min(..., 1.f)`로 막는다(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:184`).
- **제안**: Effect VM과 동일하게 `FMath::Clamp(..., 0.f, 1.f)`를 적용한다.
- **확신도**: 중간

### 9. 🟢 자유 커서 해제가 HUD의 입력 설정을 하드코딩해 이중 정의가 됐다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:76` (원본은 `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp:12`)
- **범주**: 중복/복잡도
- **문제**: `HandleFreeCursorReleased`가 `FUIInputConfig(Game, CapturePermanently)`를 직접 만들어 복원한다. 같은 값이 `UWxActivatableWidget::GetDesiredInputConfig`에도 있어, HUD WBP가 `InputMode`를 Menu로 바꾸면 복원 경로만 옛 설정으로 되돌아간다(눌렀다 뗀 뒤에야 입력 모드가 어긋나는 형태라 추적이 어렵다).
- **제안**: 복원 시 `GetDesiredInputConfig()` 결과를 그대로 쓴다(값이 없으면 아무 것도 하지 않음).
- **확신도**: 높음

### 10. 🟢 확인 팝업이 뜨지 못하면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:132`, `:138` (BP 진입은 `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:114`)
- **범주**: 설계/구조
- **문제**: `ShowConfirmation`은 레이아웃 미생성·서술자 null·팝업 클래스 미설정·로드 실패 네 경우 모두 조용히 `return`하며, 반환값도 실패 신호도 없다. BP가 `ShowConfirmationPopup`의 `OnResult`로 후속 동작을 이어 붙였다면(세이브 삭제 확인 등) 그 흐름은 아무 표시 없이 멈춘다. 팝업 클래스는 `UWxUIDeveloperSettings`의 설정값이라 프로젝트 설정 누락만으로 재현되는데, 로그조차 남지 않아 원인 추적이 어렵다.
- **제안**: 실패 경로에서 `LogWxUI` 경고를 남기고, 결과 콜백이 있으면 `EWxPopupResult::Killed`(또는 `Unknown`)로 즉시 회수해 BP 흐름이 끊기지 않게 한다.
- **확신도**: 높음(코드 사실) / 우선순위는 낮음

### 11. 🟢 인디케이터 위젯이 배열 인덱스로 묶여 있어 중간 등록이 빠지면 대상이 밀린다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp:86-89` (제거는 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp:171`)
- **범주**: 설계/구조
- **문제**: `UWxViewModel_Indicator`는 `Registered[IndicatorIndex]`로 자기 등록증을 찾고, 매니저의 `Indicators.Remove()`는 순서를 유지한 채 뒤 원소를 당긴다. 인디케이터를 둘 이상 동시에 띄운 상태에서 앞쪽 하나가 해제되면 남은 등록증의 인덱스가 전부 한 칸씩 내려가, 각 위젯이 담당하던 대상이 다른 위젯으로 옮겨간다. 위젯 외형이 같아 보통은 눈에 띄지 않지만, 위젯별 애니메이션·전환 상태는 그 순간 리셋된다. 리졸버 주석(`WxViewModel_Indicator.h:93-98`)이 "순번이 다른 위젯을 더 배치"하라고 안내하는 바로 그 다중 표시 시나리오에서만 드러난다.
- **제안**: 인덱스 대신 등록증 자체(또는 매니저가 발급하는 안정 ID)로 묶거나, 매니저가 해제 시 슬롯을 `nullptr`로 비워 뒤 원소가 밀리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 동시 표시가 1개뿐이면 발현하지 않는다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxSubtitleStateTreeNodes.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Selection.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, 및 대응 `Public/` 헤더 전반
- **확인 후 위반 0건**: 모듈 경계(`.uplugin`·`Build.cs`·include 모두 Wx 중 `WxCore`만 참조 — `WxUI.Build.cs:27`) · 전 소스 64개 첫 줄 Copyright 주석 · `Wx` prefix · `FORCEINLINE`/헤더 인라인 정의(StateTree `GetInstanceDataType` 2건은 파일 주석에 예외 근거 명시, `PushWidgetToLayerStack`은 템플릿이라 헤더 정의가 불가피) · 델리게이트 콜백의 `Handle` prefix · 람다 3건(`WxUILibrary.cpp:25` 다이나믹→네이티브 델리게이트 브릿지, `WxTabListWidgetBase.cpp:44` `FindByPredicate`, `WxUIManagerSubsystem.cpp:145` CommonUI `AddWidget` 초기화 콜백)은 모두 API상 필요한 자리
- **직전 리뷰(2026-08-03) 대비 해소 확인**: `HandleTabCreation_Implementation`의 `TabButton` 널 검사 순서는 함수 진입부 조기 반환으로 정리됨(`WxTabListWidgetBase.cpp:116-119`) · `UWxViewModel_Effect::EffectEndTime` 데드 필드 제거됨
- **미검토 / 한계**:
  - `UWxNameplateComponent::InitializeViewModels`(`WxNameplateComponent.cpp:88`)의 `InASC` 널 가드 부재는 직전 리뷰가 올렸으나 `.claude/worklog/2026-07-24-네임플레이트-표시판정-이벤트-구동화.md`에서 "유일 호출자가 기본 서브오브젝트를 넘기므로 호출자 없는 방어 가드는 지양"으로 명시적으로 기각됐다. 재차 올리지 않았으며, 소비 측이 늘어나면 재검토 대상이다.
  - `PrimaryGameLayout`·`TrackedPlayerController`가 단일 필드라 로컬 플레이어가 둘 이상이면 나중 플레이어가 앞선 플레이어의 레이아웃을 철거한다(`WxUIManagerSubsystem.cpp:248`). 코드·README·ST 노드 주석이 모두 "v1 싱글/리슨 호스트 전제"를 명시하므로 의도된 제약으로 보아 발견으로 올리지 않았다.
  - `UWxViewModel_Subtitle`이 글로벌 컬렉션(`UMVVMGameSubsystem`)에 등록된 뒤 해제되지 않아(`WxViewModel_Subtitle.cpp:42`) 자막이 걸린 채 레벨을 넘어가면 다음 월드에 남는지는 실행 확인이 필요해 판단을 보류했다. `FWxStateTreeTask_PrintSubtitle::ExitState`가 월드 정리 시 반드시 도는지가 관건이다.
  - `UWxActionWidget::GetIcon`의 에디터 전용 IMC 전수 스캔(`WxActionWidget.cpp:32`)은 디자인타임 한정이라 런타임 관점에서 깊게 보지 않았다. BP/WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)와 `UWxUIDeveloperSettings`의 실제 config 값은 범위 밖이다.

---
*문서 기준 커밋 `1e9b745c` · 리뷰일 2026-08-05 · 소스 64파일 — `/module-review`로 갱신*
