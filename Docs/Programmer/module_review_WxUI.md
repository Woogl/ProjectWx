# WxUI — 코드 리뷰

> 수명·구독 해제 규약이 주석으로 명문화되고 모듈 경계도 깨끗한(WxCore 외 Wx 참조 0건) 건강한 모듈이다. 직전 리뷰(`e9440f73`) 이후 어트리뷰트·어빌리티 VM 조회 키가 정리되고 `UWxViewModel_Selection` 이 통째로 삭제되며 지적 2건이 해소됐고, 남은 9건을 현재 코드에서 라인 단위로 재확인했다(새 발견 0건). 이번 리뷰는 `UWxUIManagerSubsystem`·뷰모델 계층 전체·인디케이터 매니저·ST 노드 2종·팝업·HUD 레이아웃·탭 리스트를 cpp까지 깊게 보고, 위젯 베이스·설정·모듈 파일은 훑었다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 팝업이 버튼 이외 경로로 닫히면 결과 콜백이 영영 오지 않는다 — `KillPopup` 은 호출부 없는 죽은 API
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:81`, `:87`
- **범주**: 버그/정확성
- **문제**: 결과 콜백이 발화하는 유일한 경로는 `HandleResultChosen`(`WxConfirmationPopup.cpp:87`)이고, 그것을 부르는 곳은 세 버튼의 `OnClicked`(`:13-24`)와 `KillPopup`(`:81`)뿐이다. 그런데 `KillPopup` 은 저장소 전체에 호출부가 없을 뿐 아니라 `UFUNCTION` 도 아니라 BP 에서조차 부를 수 없고, `NativeOnDeactivated` 처럼 "버튼 없이 닫힘"을 감지해 `EWxPopupResult::Killed`(`Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:15`)를 흘리는 훅도 없다. 즉 열거형 값 하나가 구조적으로 도달 불가다. 따라서 `UWxUILibrary::DeactivateWidgetsInLayer(..., UI.Layer.Modal)`(`Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:93` 의 `Stack->ClearWidgets()`)나 WBP 가 `bIsBackHandler` 를 켠 경우의 back 처리로 팝업이 사라지면, `ShowConfirmationPopup` 의 `OnResult` 에 이어 붙인 BP 흐름이 아무 신호 없이 그 자리에서 멈춘다.
- **제안**: `UWxConfirmationPopup::NativeOnDeactivated`(또는 `NativeDestruct`)에서 `OnResultCallback` 이 아직 바인딩돼 있으면 `Killed` 로 회수한다 — `HandleResultChosen` 이 이미 언바인딩으로 1회성을 보장하므로 버튼 경로와 중복 실행되지 않는다. 그러면 `KillPopup` 은 그 경로의 명시적 트리거로만 남는다.
- **확신도**: 높음(호출부 부재·BP 미노출은 확정. 다만 `ShowConfirmationPopup` 자체에 C++·콘텐츠 호출자가 아직 없어 현재는 잠복 상태다)

### 2. 🟡 자유 커서를 뗄 때, 그 사이 열린 메뉴의 입력 설정을 HUD 것으로 덮어쓴다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:65-79` (유발 지점 `:19-25`, `:60-62`)
- **범주**: 버그/정확성
- **문제**: `HandleFreeCursorPressed` 가 `SetActiveUIInputConfig(All, NoCapture)` 로 활성 입력 설정을 직접 갈아끼운다(`:60-62`). 그 상태에서 CommonUI 의 액션 매칭식은 `ActiveInputMode == All || ActiveInputMode == Binding->InputMode`(엔진 `UIActionRouterTypes.cpp:741`·`:852` 에서 확인)이므로, `InputMode = Game` 으로 등록된 Inventory·MainMenu 바인딩(`:19-25`)이 **자유 커서를 쥔 채로도 매칭된다**. 커서를 든 상태로 인벤토리를 열면 엔진이 그 메뉴의 설정(보통 Menu/NoCapture)을 적용하는데, 이어서 커서 키를 떼면 `HandleFreeCursorReleased` 가 HUD 자신의 희망 설정(Game/CapturePermanently)을 조건 없이 밀어 넣어(`:77`) 열려 있는 메뉴 위에 게임 입력 모드·마우스 캡처가 걸린다. 엔진은 다음 활성화 변화가 올 때까지 재적용하지 않으므로 메뉴를 닫기 전까지 마우스 조작이 막힌 상태가 유지된다.
- **제안**: 뗄 때 무조건 HUD 설정을 밀지 말고, 자기가 눌러서 바꾼 경우에만 되돌린다 — 그 사이 다른 위젯이 설정을 가져갔으면 복원을 건너뛰고 `ActionRouter->RefreshUIInputConfig()` 로 엔진이 다시 계산하게 맡긴다. 또는 자유 커서 홀드 중 메뉴 액션이 매칭되지 않도록 홀드 입력 모드를 `Game` 으로 유지한다.
- **확신도**: 중간(엔진 매칭식·적용 시점은 확인했으나, 실제 발현은 HUD WBP 가 두 액션을 같은 화면에 함께 두는지에 달림)

### 3. 🟡 어빌리티 발동 가능 재평가가 제네릭 태그 이벤트와 매 틱 GE 질의에 얹혀 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:38`, `:341`, `:380`
- **범주**: 성능/안전
- **문제**: (a) VM 마다 `RegisterGenericGameplayTagEvent()` 를 구독해(`:38`) ASC 의 **모든** 태그 변경 1건마다 `RefreshActivationState`(`:305`)를 돌린다. 이 함수는 `GetActivatableAbilities()` 선형 탐색(`:396`)에 `CanActivateAbility` + `CheckCost`(`:406-407`)를 얹으므로 비용이 `VM 수 × 태그 변경 수`로 곱해진다. 전투 중 태그가 초당 수십 번 갈리는 캐릭터에서 HUD 슬롯 수만큼 배가되며, 같은 ASC 를 보는 `UWxViewModel_AbilitySystem` 도 제네릭 구독을 하나 더 걸어(`WxViewModel_AbilitySystem.cpp:41`) 매 태그 변경마다 `GetOwnedGameplayTags` 로 컨테이너를 통째로 새로 만든다(`:176`). (b) 쿨다운이 도는 동안에는 매 틱 `ASC->GetActiveEffects(Query)`(`:341`)가 `TArray` 를 새로 할당해 전 활성 GE 를 훑고, 그 위에 다시 `RefreshActivationState`(`:380`)를 부른다 — `CanActivateAbility` 내부가 쿨다운·비용을 또 조회하므로 GE 순회가 프레임마다 겹친다.
- **제안**: 태그 구독을 해당 어빌리티의 요건 태그(`ActivationRequiredTags`/`ActivationBlockedTags`) 단위 `RegisterGameplayTagEvent` 로 좁힌다. 매 틱 재평가는 이미 알고 있는 "가장 이른 만료 시각"에 맞춰 예약하거나 갱신 주기를 0.1초로 낮춘다 — 표시용이라 한두 프레임 지연은 문제되지 않는다.
- **확신도**: 중간(프로파일링 없이 코드 형태로만 판단)

### 4. 🟡 규칙 위반 — 탭 리스트 위젯의 `BlueprintCallable` 7건
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`, `:72`, `:75`, `:78`, `:81`, `:84`, `:87`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리로 제한한다. `UWxTabListWidgetBase` 의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)가 범위 밖이며, 그중 3개는 `BlueprintCallable, BlueprintPure` 를 함께 붙여 지정자 자체가 중복이다(`BlueprintPure` 가 호출 가능을 함의). Lyra `ULyraTabListWidgetBase` 이식 시 원본 지정자가 그대로 넘어온 것으로 보인다. (`UWxUILibrary` 4건과 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 는 규칙이 허용하는 자리, `UWxButtonBase::SetButtonText` 는 MVVM 목적지가 요구하는 1-arg setter, `UWxViewModel_Ability::TryActivateAbility` 는 VM Command 예외라 모두 제외했다.)
- **제안**: 순수 조회 5개는 `BlueprintPure` 만 남기고, 변경 함수 2개(`SetTabHiddenState`·`RegisterDynamicTab`)는 BP 노출이 실제로 필요한지 확인해 필요하면 `UWxUILibrary` 파사드로 옮긴다.
- **확신도**: 높음(규칙 문면상 위반은 확정)

### 5. 🟢 쿨다운 퍼센트만 클램프되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:376`
- **범주**: 버그/정확성
- **문제**: `SetCooldownPercent(NextChargeRemaining / CooldownDuration)` 에 상한이 없다. `CooldownDuration` 은 그 사이클의 첫 적용 시점에만 기록되고(`:284-288`, 시드는 `:70-75`) 충전이 모두 회복될 때까지 갱신되지 않으므로, 도중에 더 긴 쿨다운이 적용되면 1을 초과한 값이 프로그레스 바로 흘러간다. 동일 패턴인 `UWxViewModel_Effect::UpdateEffectState` 는 `FMath::Min(..., 1.f)` 로 막는다(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:175`).
- **제안**: Effect VM 과 동일하게 `FMath::Clamp(..., 0.f, 1.f)` 를 적용한다.
- **확신도**: 중간

### 6. 🟢 무한 지속 이펙트는 스택 수가 갱신되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:34` (초기 세팅 `:31`, 유일한 갱신 경로 `:162`)
- **범주**: 버그/정확성
- **문제**: `StackCount` 를 갱신하는 곳은 `UpdateEffectState`(`:162`) 하나뿐인데, 그 티커는 `CachedDuration > 0.f` 일 때만 등록된다(`:34`, `:49`). 무한 지속 GE 는 `GetDuration()` 이 `INFINITE_DURATION`(엔진에서 `-1.f`)을 돌려주므로 티커가 아예 붙지 않고, 스택이 쌓여도 `Initialize` 시점 값에 얼어붙는다. 엔진은 스택만 늘어날 때 `OnStackCountChange` 만 부르고 `OnActiveGameplayEffectAddedDelegateToSelf` 는 새 활성 GE 일 때만 발행하므로(`GameplayEffect.cpp:4548-4557`), `UWxViewModel_AbilitySystem::HandleActiveEffectAdded`(`WxViewModel_AbilitySystem.cpp:185-202`)로 VM 이 재생성되지도 않는다. `TimeRemainingPercent` 도 0에 머물러 위젯이 "만료됨"으로 읽을 여지가 있다.
- **제안**: `ASC->OnGameplayEffectStackChangeDelegate(BoundHandle)` 를 구독해 스택 변경을 이벤트로 받는다 — 지속시간 유무와 무관하게 성립하며 매 틱 폴링도 필요 없다.
- **확신도**: 낮음(코드 경로는 확정이나, 현재 `UWxEffectComponent_UIData` 를 붙인 GE 가 `GE_Burn` 하나뿐이라 실사용 발현은 미확인)

### 7. 🟢 팝업·비동기 push 의 실패 경로가 아무 신호를 내지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:96-116` / `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:96-113` / `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:43-104`
- **범주**: 설계/구조
- **문제**: `ShowConfirmation` 은 레이아웃 미생성·서술자 null·팝업 클래스 미설정·로드 실패 네 경우 모두 조용히 `return` 하며 반환값도 실패 신호도 없다(`:99-108`). BP 가 `ShowConfirmationPopup` 의 `OnResult` 로 후속 동작을 이어 붙였다면 그 흐름은 아무 표시 없이 멈춘다(발견 1과 같은 증상, 다른 원인이다). 팝업 클래스는 `UWxUIDeveloperSettings` 설정값이라 프로젝트 설정 누락만으로 재현되는데 로그조차 남지 않는다. `UWxAsyncAction_PushWidgetToLayer::HandleWidgetClassLoaded` 도 실패 지점 7곳 전부가 `BeforePush`/`AfterPush` 없이 `SetReadyToDestroy()` 만 하므로, BP 비동기 노드의 두 출력 핀이 모두 발화하지 않고 그래프가 그 자리에서 끊긴다.
- **제안**: 실패 경로에서 `LogWxUI` 경고를 남기고, 팝업은 결과 콜백이 있으면 `EWxPopupResult::Killed` 로 즉시 회수한다. 비동기 액션은 실패 출력 핀을 추가하거나 최소한 경고를 남긴다.
- **확신도**: 높음(코드 사실) / 우선순위는 낮음

### 8. 🟢 인디케이터 위젯이 배열 인덱스로 묶여 있어 등록 순서가 곧 표시 대상이 된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp:84-88` (제거는 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp:167`)
- **범주**: 설계/구조
- **문제**: `UWxViewModel_Indicator` 는 `Registered[IndicatorIndex]` 로 자기 등록증을 찾는데, 매니저의 `Indicators` 는 등록 순서 배열이고 `Remove()` 는 순서를 유지한 채 뒤 원소를 당긴다. `FWxStateTreeTask_MarkIndicator` 가 인스턴스마다 등록증 1개를 발급하므로(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:67`) 동시에 여러 태스크가 도는 순간 인덱스는 "먼저 해석에 성공한 순서"가 된다 — 앞 대상이 스트리밍 아웃돼 있으면 뒤 대상이 0번을 차지하고, 나중에 앞 대상이 로드되면 1번에 붙는다. 해제 시에도 뒤 원소가 당겨져 각 위젯이 담당하던 대상이 서로 바뀐다. 위젯 외형이 같으면 눈에 띄지 않지만, 위젯별 애니메이션·전환 상태는 그 순간 리셋된다.
- **제안**: 인덱스 대신 등록증 자체(또는 매니저가 발급하는 안정 ID)로 묶거나, 매니저가 해제 시 슬롯을 `nullptr` 로 비워 뒤 원소가 밀리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 리졸버의 `IndicatorIndex` 프로퍼티 주석이 "순번이 다른 위젯을 더 배치"로 안내하므로 순서 불안정을 감수한 설계로 볼 여지가 있다)

### 9. 🟢 티커에 바인딩되는 콜백 2개가 `Handle` prefix 규칙 밖에 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:89`, `:295` (`UpdateCooldownState`, 정의 `:314`) / `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:50` (`UpdateEffectState`, 정의 `:145`)
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4는 "Delegate 에 바인딩되는 Callback 함수는 `Handle` 을 Prefix 로 사용한다"이고, 두 함수는 `FTickerDelegate::CreateUObject` 로 델리게이트에 바인딩된다. 모듈의 나머지 콜백은 전부(GE 적용·태그 변경·어트리뷰트 변경·스트리밍 완료·위젯 활성화·빙의 교체) 규칙을 지키고 있어 이 둘만 결이 다르다.
- **제안**: 이름만 `HandleCooldownTick`/`HandleEffectTick` 류로 맞추거나, "주기 갱신 함수는 이벤트 콜백이 아니다"를 예외로 볼지 결정해 규칙 해석을 한 번 고정한다.
- **확신도**: 낮음(틱 함수를 이벤트 콜백으로 볼지에 대한 해석 문제 — 앞선 리뷰들이 서로 다르게 판단했다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`, 및 대응 `Public/` 헤더 전반
- **직전 리뷰 이후 해소 확인(2건)**: 어트리뷰트·어빌리티 VM 캐시 키가 요청을 온전히 식별하지 못하던 문제 — 어트리뷰트는 `(Current, Max)` 쌍으로 조회하고 Max 무효 시 Current 로 정규화한 뒤 비교하며(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:76-85`), 어빌리티는 요청 태그가 아니라 매칭된 어빌리티 CDO 로 먼저 확정한 뒤 그 CDO 로 캐시를 뒤져(`:105-126`) 생성 순서 의존이 사라졌다. `UWxViewModel_Selection` 미배선 — VM·리졸버·서브시스템의 생성·등록·접근자가 전부 삭제돼 현재 소스에 존재하지 않는다.
- **확인 후 위반 0건**: 모듈 경계(`.uplugin`·`Build.cs`·전 include 목록 모두 Wx 중 `WxCore` 만 참조 — `Plugins/WxUI/Source/WxUI/WxUI.Build.cs:26`) · 전 소스 61개 첫 줄 Copyright 주석 · `Wx` prefix · `FORCEINLINE` 0건(헤더 정의는 StateTree `GetInstanceDataType` 2건과 템플릿 `PushWidgetToLayerStack` 1건뿐이며 앞의 둘은 파일 주석에 예외 근거 명시) · override 의 `Super::` 호출 누락 없음(`ApplyLoadedImage` 파생 3건은 베이스가 의도된 빈 수신점) · 람다 3건(`Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:24` 다이나믹→네이티브 델리게이트 브릿지, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:111` CommonUI `AddWidget` 초기화 콜백, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43` `FindByPredicate`)은 모두 API 상 필요한 자리로 직전 리뷰와 동일 판단
- **미검토 / 한계**:
  - `UWxViewModel_AbilitySystem` 은 ASC 를 Outer 로만 삼을 뿐 아무도 강참조하지 않는다(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:23`, 자식도 ASC 를 Outer 로 만든다 — `:87`, `:128`). UE 의 GC 는 Outer 를 자식→부모 방향 참조로만 다루므로(`FastReferenceCollector.h:1005-1019`), 어트리뷰트·어빌리티 리졸버처럼 자식 VM 만 반환하는 경로(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp:158-160`)만 살아 있으면 Composite 가 수거돼 공유가 조용히 깨질 수 있다. 다만 현재 콘텐츠는 `WBP_GameHUD`·나머지 네임플레이트가 `UWxViewModel_Character`(`AbilitySystem` 을 `UPROPERTY` 로 붙듦)를 함께 만들어 앵커가 서 있어 발현하지 않는다 — 앞으로 Character VM 없이 어트리뷰트/어빌리티 VM 만 쓰는 화면을 만들 때 재검토가 필요하다.
  - `UWxViewModel_Character::Deinitialize`(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:29-37`)는 `FieldNotify` 프로퍼티 3개를 브로드캐스트 없이 비운다. 현재 `Initialize` 를 두 번 부르는 호출자가 없어 소멸 시점에만 도달하므로 발견으로 올리지 않았다.
  - `UWxNameplateComponent::InitializeViewModels`(`Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:89`)의 `InASC` 널 가드 부재는 `.claude/worklog/2026-07-24-네임플레이트-표시판정-이벤트-구동화.md` 에서 "유일 호출자가 기본 서브오브젝트를 넘기므로 호출자 없는 방어 가드는 지양"으로 기각됐다. 호출자가 `Source/WxGame/Character/WxEnemyCharacter.cpp:40` 하나뿐임을 재확인했고, 같은 함수를 재호출하면 태그 이벤트를 중복 구독하는 것도 호출자가 1회성이라 제외했다.
  - `PrimaryGameLayout`·`TrackedPlayerController` 가 단일 필드라 로컬 플레이어가 둘 이상이면 나중 플레이어가 앞선 플레이어의 레이아웃을 철거한다(`WxUIManagerSubsystem.cpp:195-228`). 같은 이유로 `FWxStateTreeTask_MarkIndicator` 가 0번 컨트롤러에서만 매니저를 찾는다(`WxStateTreeTask_MarkIndicator.cpp:26`). 코드·README·ST 노드 주석이 모두 "v1 싱글/리슨 호스트 전제"를 명시하므로 의도된 제약으로 보아 발견으로 올리지 않았다. 정지 재평가가 `NM_Standalone` 한정인 것(`WxUIManagerSubsystem.cpp:154`)도 같은 전제이며, 프로젝트 내 `SetGamePaused` 호출자가 이 한 곳뿐이라 외부 정지와 충돌하지 않음을 확인했다.
  - `UWxViewModel_Subtitle` 이 글로벌 컬렉션(`UMVVMGameSubsystem`)에 등록된 뒤 해제되지 않아(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp:42-43`) 자막이 걸린 채 레벨을 넘어가면 다음 월드에 남는지는 실행 확인이 필요해 판단을 보류했다. `FWxStateTreeTask_PrintSubtitle::ExitState` 가 월드 정리 시 반드시 도는지가 관건이다.
  - `UWxActionWidget::GetIcon` 의 에디터 전용 IMC 전수 로드·스캔(`Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:32-36` — `GetAssetsByClass` 후 매 IMC 에 `GetAsset()` 동기 로드)은 디자인타임 한정이라 런타임 관점에서 깊게 보지 않았다. IMC 가 많아지면 위젯 에디터가 무거워질 수 있다는 것만 기록해 둔다.
  - `UWxMVVMConversionLibrary::GetAttributeViewModel` 이 `BlueprintPure` 인데 VM 을 생성하는 부수효과를 갖는 것(`Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h:29`)은 지연 생성이라는 명시적 설계라 발견으로 올리지 않았다.
  - BP/WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 61파일 — `/module-review`로 갱신*
