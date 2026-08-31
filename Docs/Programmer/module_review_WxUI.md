# WxUI — 코드 리뷰

> 전반적으로 건강하다. 모듈 경계(WxCore 외 Wx 참조 없음)가 깨끗하고, 수명주기 해제·재진입·비동기 취소 같은 까다로운 지점이 대부분 의도적으로 처리돼 있으며 그 근거가 주석으로 남아 있다. 이번 리뷰는 오케스트레이션(UIManagerSubsystem)·MVVM 뷰모델 계층·비동기 push 액션·인디케이터 매니저·StateTree 태스크의 cpp 까지 깊게 보고, CommonUI 위젯 베이스류(탭·버튼·액션 위젯)와 설정·모듈 파일은 훑었다. 심각(🔴) 등급의 결함은 찾지 못했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 최대치 미지정 어트리뷰트 VM 은 퍼센트가 항상 1.0, 가득참이 항상 true 로 굳는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:81`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp:27`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp:142`
- **범주**: 버그/정확성
- **문제**: `GetOrCreateAttributeViewModel` 은 `Max` 가 무효면 `Current` 자신을 최대치로 삼는다. 그 결과 `MaxAttributeAmount == AttributeAmount` 가 되어 `RecalculateAttributePercent` 는 언제나 1.0 을, `SetIsAttributeFull` 은 언제나 true 를 내놓는다. 리졸버(`UWxViewModelResolver_Attribute`)에서 `MaxAttribute` 를 비워 둔 채 게이지 위젯을 `AttributePercent` 에 바인딩하면 값과 무관하게 항상 가득 찬 바가 그려지는데, "최대치를 안 쓰는 표시"를 의도한 저작자가 이 부작용을 예측하기 어렵다. 덤으로 이때 `Initialize` 의 두 등록(`WxViewModel_Attribute.cpp:28`, `:36`)이 같은 어트리뷰트의 같은 델리게이트에 걸려, 값 변화 한 번에 `HandleAttributeChanged`·`HandleMaxAttributeChanged` 가 모두 돌며 `RecalculateAttributePercent` 가 중복 실행된다.
- **제안**: 최대치가 없는 경우를 "최대치 없음"으로 명시해 다루는 편이 낫다 — `BoundMaxAttribute` 를 무효로 남기고, 퍼센트·가득참 필드는 갱신하지 않거나 0 으로 두어 바인딩 측이 쓸 수 없음을 알 수 있게 한다. 최소한 같은 어트리뷰트일 때는 Max 쪽 등록을 건너뛴다.
- **확신도**: 중간 (Current 를 최대치로 쓰는 것 자체는 헤더에 명시된 의도이나, 파생 필드가 상수로 굳는 결과까지 의도된 것인지는 불분명)

### 2. 🟡 메뉴·팝업·레이아웃 위젯 클래스를 동기 로드한다 — 비동기 push 경로를 두고도
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:94`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:95`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:333`
- **범주**: 성능/안전
- **문제**: 이 모듈은 소프트 클래스를 비동기로 스트리밍해 push 하는 `UWxAsyncAction_PushWidgetToLayer` 를 갖추고 있고 HUD·사망 화면·대화 창은 그 경로로 뜬다. 그런데 인벤토리/메인 메뉴 토글(`PushMenuWidget`)과 확인 팝업(`ShowConfirmation`)은 `LoadSynchronous()` 로 클래스를 끌어온다. 특히 `PushMenuWidget` 은 플레이 중 입력 프레임에서 도는 경로라, 인벤토리 WBP 와 그 하위 참조(아이콘·서브위젯)가 아직 메모리에 없으면 그 프레임이 통째로 멈춘다. 메뉴가 커질수록 악화된다.
- **제안**: `PushMenuWidget`·`ShowConfirmation` 도 `UWxAsyncAction_PushWidgetToLayer` 로 통일한다(팝업은 `SetCompletionCallback` 으로 `SetupPopup` 을 잇는다). 레이아웃 생성(`CreateLayoutForPlayer`)은 빈 컨테이너라 동기 로드로 남겨도 무방하다.
- **확신도**: 중간

### 3. 🟡 어빌리티 VM 은 "늦게 부여된 어빌리티"를 복구할 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:109`
- **범주**: 설계/구조
- **문제**: `GetOrCreateAbilityViewModel` 은 호출 시점의 `GetActivatableAbilities()` 에서 태그가 맞는 스펙을 찾지 못하면 nullptr 를 반환하고, 리졸버는 위젯 생성 시 단 한 번만 돈다. MVVM 리졸버가 돌려준 인스턴스는 나중에 교체할 수 없으므로, 어빌리티 부여가 위젯 생성보다 늦으면 그 슬롯은 위젯이 다시 만들어질 때까지 영영 비어 있다. 같은 문제를 겪는 인디케이터 VM 은 인스턴스를 고정한 채 도착 신호(`OnAnyManagerReady`, `WxViewModel_Indicator.cpp:26`)로 내부 상태만 갈아끼우는 패턴을 쓰는데, 어빌리티 VM 에는 그 대응이 없다. 지금은 `PossessedBy` 가 `OnPossessedPawnChanged` 보다 먼저 돌아 스탠드얼론에서 문제가 드러나지 않지만, ActivatableAbilities 가 뒤늦게 복제되는 네트워크 클라이언트에서는 스킬 아이콘이 통째로 비게 된다.
- **제안**: 어빌리티 VM 도 인스턴스 고정 + 늦은 바인딩으로 맞춘다 — VM 을 태그로 먼저 만들어 두고, ASC 의 어빌리티 부여 통지(`AbilitySystemComponent->AbilityCommittedCallbacks` 대신 `OnGiveAbility`/`ActivatableAbilities` 변경)에서 실제 CDO 를 잡아 채우는 형태. 당장 고치지 않더라도 v1 이후 멀티 대응 시 반드시 다시 볼 지점으로 남긴다.
- **확신도**: 중간 (v1 싱글/리슨 호스트 전제에서는 현재 증상이 나타나지 않음)

### 4. 🟡 위젯 클래스에 `BlueprintCallable` 이 붙어 있다 (코딩 규칙 5 위반)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:65,71,74,77,80,83,86`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20`
- **범주**: 규칙 위반
- **문제**: `BlueprintCallable` 은 Blueprint Function Library 와 Blueprint Async Action 팩토리(그리고 관례상 ViewModel 의 Command 함수)에서만 쓰기로 돼 있는데, `UWxTabListWidgetBase` 의 7개 함수와 `UWxButtonBase::SetButtonText` 가 위젯 클래스에서 직접 노출돼 있다. 이 파일들은 Lyra 계열 베이스를 옮겨온 것으로 보이며, 그 지정자까지 함께 따라왔다.
- **제안**: 조회 계열(`GetPreregisteredTabInfo`, `IsFirstTabActive`, `IsLastTabActive`, `IsTabVisible`, `GetVisibleTabCount`)은 `BlueprintPure` 단독으로 내리고, 상태를 바꾸는 `SetTabHiddenState`·`RegisterDynamicTab`·`SetButtonText` 는 WBP 가 실제로 부르는지 확인해 필요하면 `UWxUILibrary` 로 옮기거나 지정자를 제거한다. (참고로 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 와 `UWxUILibrary` 의 지정자는 규칙에 맞는다.)
- **확신도**: 높음

### 5. 🟢 티커 델리게이트에 바인딩되는 콜백에 `Handle` 접두가 없다 (코딩 규칙 4)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:50`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:314`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:226`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:56`
- **범주**: 규칙 위반
- **문제**: `UpdateCooldownState`, `FlushActivationRefresh`, `FlushOwnedTagsRefresh`, `UpdateEffectState` 가 `FTickerDelegate::CreateUObject` 로 바인딩되는데 `Handle` 접두가 없다. 모듈의 다른 델리게이트 콜백은 전부 규칙을 지키고 있어 이 넷만 튄다. 다만 `UpdateCooldownState` 는 `SetMaxRecharges` 에서 직접 호출되기도 해 순수한 콜백은 아니다.
- **제안**: 티커 진입점을 `HandleCooldownTick` 처럼 분리하고, 직접 호출이 필요한 계산 본체는 별도 이름으로 두거나 — 티커를 규칙 대상에서 제외할 생각이라면 CLAUDE.md 쪽에 예외를 명시한다.
- **확신도**: 중간 (이벤트 델리게이트가 아니라 티커라 규칙 적용 범위가 애매)

### 6. 🟢 `GetCost` 는 이름과 달리 상태를 바꾸고 구독까지 건다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:417` (호출부 `:34`)
- **범주**: 중복/복잡도
- **문제**: `GetCost(InASC, InAbility);` 는 호출부에서 반환값을 버리는 조회처럼 읽히지만, 실제로는 `CostAttribute`·`CostMaxAttribute` 를 정하고 `CostAmount` 를 세팅하며 어트리뷰트 변경 델리게이트 두 개를 등록한다. 나중에 이 줄을 "값만 읽는 줄"로 오해하고 옮기거나 지우면 구독이 통째로 사라진다.
- **제안**: `ResolveCostAttributes` 처럼 하는 일을 드러내는 이름으로 바꾼다.
- **확신도**: 높음

### 7. 🟢 팝업 강제 종료 경로에 호출자가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80`
- **범주**: 중복/복잡도
- **문제**: `UWxGamePopup::KillPopup` 은 프로젝트 어디에서도 호출되지 않는다(C++ 전 범위 확인). 따라서 `UWxConfirmationPopup::KillPopup` 도, 그것이 내는 `EWxPopupResult::Killed` 도 도달 불가다. 결과 콜백을 받는 쪽은 `Killed` 를 처리할 이유가 없는데 열거형에는 남아 있어 계약이 실제보다 넓어 보인다.
- **제안**: 사용자 입력 없이 팝업을 걷는 시나리오(레벨 전환·세이브 로드 등)가 계획에 있다면 그때 호출부를 붙이고, 없다면 `KillPopup` 과 `Killed` 를 함께 걷는다.
- **확신도**: 높음

### 8. 🟢 디자인타임 아이콘 해석이 프로젝트의 모든 IMC 를 동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29`
- **범주**: 성능/안전
- **문제**: `GetIcon()` 은 디자인타임에 애셋 레지스트리로 `UInputMappingContext` 전부를 열거한 뒤 `MappingContextAsset.GetAsset()` 으로 하나씩 동기 로드해 매핑을 훑는다. 캐시가 없고 조기 종료도 매칭된 뒤에만 일어나므로, 아이콘이 다시 그려질 때마다(스타일 변경·PreConstruct) 이 스캔이 반복된다. 에디터 전용이라 런타임 영향은 없지만 IMC 가 늘수록 WBP 편집이 무거워진다.
- **제안**: 액션→키 해석 결과를 정적 맵에 캐시하고 애셋 등록/변경 시에만 무효화하거나, 최소한 `FAssetData` 태그로 후보를 좁힌 뒤 로드한다.
- **확신도**: 중간

### 9. 🟢 매 프레임 반복되는 조회 두 곳
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:45`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:107`
- **범주**: 성능/안전
- **문제**: (a) 네임플레이트는 인스턴스마다 매 틱 `GetFirstPlayerController()` + `GetPlayerViewPoint()` 를 부른다 — 화면에 적이 여럿이면 같은 카메라 값을 개수만큼 중복 조회한다. (b) `FWxStateTreeTask_MarkIndicator::Tick` 은 대상을 아직 잡지 못한 동안 매 프레임 `Locator.SyncFind` 로 재해석을 시도한다(백오프 없음). 스트리밍 아웃된 먼 목표를 가리키는 동안 계속 도는 경로다.
- **제안**: (a) 카메라 위치를 프레임당 한 번 구해 공유하거나, 스케일 갱신 주기를 낮춘다. (b) 재시도에 간격(예: 0.25s 누적)을 둔다.
- **확신도**: 중간 (둘 다 현재 규모에서는 체감되지 않을 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, 그리고 대응하는 `Public/` 헤더 전부
- **확인된 준수 사항**: 모듈 의존성은 `WxCore` + 엔진뿐이라 플러그인 DAG 규칙을 지킨다. `FORCEINLINE`·인라인 정의는 없고, 헤더 정의 두 곳(`WxPrimaryGameLayout.h:31` 템플릿, StateTree `GetInstanceDataType()`)은 예외 사유가 주석으로 명시돼 있다. 모든 소스 첫 줄에 저작권 주석이 있고(단 `WxViewModel_Ability.cpp` 만 UTF-8 BOM 이 붙어 있다), 확인한 모든 override 가 필요한 곳에서 `Super::` 를 부른다.
- **미검토 / 한계**: WBP 위젯 계층과 MVVM 바인딩 그래프(리뷰 범위 밖). `UWxTabListWidgetBase`·`UWxButtonBase`·`UWxActionWidget`·`UWxTabButtonBase` 는 CommonUI/Lyra 베이스를 옮겨온 성격이라 엔진 계약과의 정합성까지는 확인하지 않았고, 실제로 어떤 WBP 가 이들을 상속해 쓰는지도 확인하지 않았다(따라서 미사용 여부는 판단하지 않았다). 인디케이터 화면 좌표 투영·클램프 수식(`WxIndicatorManagerComponent.cpp:21-94`)은 코드를 읽었을 뿐 실제 화면 결과로 검증하지 않았다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 63파일 — `/module-review`로 갱신*
