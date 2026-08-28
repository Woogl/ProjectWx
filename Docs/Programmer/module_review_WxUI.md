# WxUI — 코드 리뷰

> 건강한 모듈이다. 뷰모델마다 `Deinitialize` 가 자기 구독을 대칭으로 끊고, 재진입·위젯 풀 재사용 같은 함정을 주석으로 짚어 두었으며 모듈 경계도 깨끗하다(`WxUI.Build.cs` 의 Wx 의존은 `WxCore` 하나, 소스의 Wx 크로스 include 는 `WxGameplayTags.h`·`WxLocatorUtils.h` 뿐). 이번 리뷰는 63개 소스를 전부 훑고 서브시스템·레이아웃 수명주기, HUD/네임플레이트 컴포넌트, VM 계층 전체(구독↔해제 1:1 대조), 인디케이터 투영·슬롯 관리, StateTree 노드, 팝업 결과 전달 경로까지 cpp 로 내려가 재확인했다. 직전 리뷰(08-27)에서 지적된 이펙트 스택 통지·인디케이터 슬롯 밀림·HUD 재-push 게이트·네임플레이트 재초기화·이펙트 VM 생성 중복·태그 이벤트 팬아웃·인디케이터 UOL 매 틱 해석은 현재 코드에서 모두 해소된 것을 확인했고, 아래는 지금 코드에 남아 있는 것만 적었다. 심각 등급 발견은 없다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 팝업이 버튼 외 경로로 닫히면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:86-94`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:78-93`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:215-219`
- **범주**: 버그/정확성
- **문제**: 결과 전달 경로는 `HandleResultChosen` 하나뿐이고 그 진입점은 버튼 `OnClicked`(`WxConfirmationPopup.cpp:12-23`)과 `KillPopup` 둘이다. 그런데 `KillPopup` 은 저장소 전체에 호출자가 없다 — 선언·정의·`Super` 위임이 전부이고 BP 노출도 없어 `EWxPopupResult::Killed` 는 도달 불가 값이다. 반면 팝업을 버튼 없이 닫는 길은 열려 있다: `UWxUILibrary::DeactivateWidgetsInLayer` 의 `Stack->ClearWidgets()`, 컨트롤러 교체 시 레이아웃 `RemoveFromParent`, CommonUI back 입력, WBP 의 직접 `DeactivateWidget`. 그 경로로 닫히면 `OnResultCallback` 은 바운드된 채 버려지고 호출자는 아무 결과도 받지 못한다. `UWxConfirmationPopup` 에 `NativeOnDeactivated` 오버라이드가 없어 걷히는 순간을 잡지도 못한다.
- **제안**: `UWxConfirmationPopup::NativeOnDeactivated` 를 오버라이드해 `OnResultCallback.IsBound()` 면 `HandleResultChosen(EWxPopupResult::Killed)` 를 태운다. 정상 경로는 이미 언바인딩 후 `DeactivateWidget` 을 부르므로 중복 발화하지 않으며, 그러면 호출자 없는 `KillPopup` 은 지워도 된다.
- **확신도**: 높음(코드 경로는 확실하다. 다만 `ShowConfirmation`/`ShowConfirmationPopup` 의 C++·BP 호출자가 아직 없어 실피해는 잠재 상태다)

### 2. 🟡 폰 상태 태그 관찰이 현재 태그를 시드하지 않아 "따라잡기"가 반쪽이다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:241-268`(등록은 263-267), 대비 지점 `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:232-233`
- **범주**: 버그/정확성
- **문제**: `WatchPawnTags` 는 `Ability.Death`·`State.Dialogue` 를 `NewOrRemoved` 로 구독만 하고 현재 보유 여부는 한 번도 읽지 않는다. 이 이벤트는 변화 시에만 발화하므로, 관찰을 시작하는 시점에 폰이 이미 태그를 들고 있으면 사망 화면·대화 창이 영영 뜨지 않는다. 232-233행 주석이 "이미 빙의를 마친 PC 일 수 있어 지금 폰으로 따라잡는다"고 선언한 바로 그 경로인데, 따라잡는 것은 구독뿐이고 상태는 아니다. 게다가 같은 함수가 진입부에서 `CloseDialogueScreen()`(253행)을 무조건 부르므로, 폰 교체가 일어나면 대화 창은 닫히기만 하고 새 폰이 `State.Dialogue` 를 갖고 있어도 다시 열리지 않는다. 같은 모듈의 다른 두 곳은 이 시드를 명시적으로 한다 — `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:133-134`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:43-44`.
- **제안**: 구독 직후 `AbilitySystem->HasMatchingGameplayTag(...)` 로 두 태그를 한 번 읽어 `HandleDeathTagChanged`/`HandleDialogueTagChanged` 와 같은 판정을 태운다(핸들러 본문을 그대로 재사용하면 규칙이 갈리지 않는다).
- **확신도**: 중간(대화·사망 중 폰 교체나 사망 상태로의 빙의가 현재 콘텐츠에 없다면 잠재 결함이다)

### 3. 🟡 `BlueprintCallable` 사용 규칙 위반 8곳(탭 리스트·버튼)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66-88`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20-21`
- **범주**: 규칙 위반
- **문제**: 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리에만 허용한다. `UWxUILibrary`·`UWxMVVMConversionLibrary`·`UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 는 적법하고, 뷰모델의 커맨드 `UWxViewModel_Ability::TryActivateAbility` 는 합의된 예외다. 남는 위반은 위젯 클래스의 8개 — `UWxTabListWidgetBase` 의 `GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount` 와 `UWxButtonBase::SetButtonText` 다. 탭 리스트 4개는 `BlueprintCallable, BlueprintPure` 를 병기해 중복 지정이기도 하다. 모두 Lyra 이식 잔재로 보인다.
- **제안**: BP 에서 실제로 쓰는 것만 남기고, 조회 함수는 `BlueprintPure` 단독으로 줄인다. 나머지는 지정자를 떼거나 필요한 진입점만 `UWxUILibrary` 정적 함수로 옮긴다.
- **확신도**: 높음(규칙 대조는 확실. 어떤 것이 WBP 그래프에서 실제로 호출되는지는 에셋 범위 밖이라 정리 대상 선별에는 확인이 필요하다)

### 4. 🟢 델리게이트 관련 네이밍 규칙 위반 6건
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:55-57`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:64-66`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:320-322`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:227-229`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:90-91`
- **범주**: 규칙 위반
- **문제**: 규칙 4(델리게이트 콜백은 `Handle` 접두사)에 어긋나는 `FTickerDelegate` 바인딩이 4건이다 — `UpdateEffectState`·`UpdateCooldownState`·`FlushActivationRefresh`·`FlushOwnedTagsRefresh`. 규칙 1(엔진 기본 Prefix + `Wx`)에 어긋나는 델리게이트 타입이 2건이다 — `FOnTabContentCreated`·`FOnTabContentCreatedNative`. 모듈 내 다른 델리게이트(`FWxPopupResultDelegate`·`FWxOnIndicatorsUpdated`·`FWxOnIndicatorManagerReady`·`FWxPushWidgetToLayerDelegate`·`FWxPopupResultDynamicDelegate`)는 전부 `FWx` 로 통일돼 있어 이 둘만 튄다.
- **제안**: 델리게이트 타입은 `FWxOnTabContentCreated`·`FWxOnTabContentCreatedNative` 로 개명한다. 티커 콜백은 `HandleCooldownTick` 류로 바꾸되, `SeedActiveCooldown` 이 직접 호출하기도 하는 `UpdateCooldownState` 는 얇은 `Handle...` 래퍼를 두는 편이 호출부를 덜 흔든다.
- **확신도**: 높음(접두사) / 중간(티커를 "델리게이트 콜백"으로 볼지에는 해석 여지가 있다)

### 5. 🟢 불필요한 람다 2건
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:23-27`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43-46`
- **범주**: 규칙 위반
- **문제**: 규칙 3(람다는 반드시 필요할 때만). 동적 델리게이트 → 네이티브 델리게이트 브리지는 `FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName())` 로 람다 없이 같은 약참조 의미를 얻는다. `FindByPredicate` 람다도 단순 range-for 로 대체 가능하며(바로 아래 `SetTabHiddenState` 가 같은 탐색을 range-for 로 이미 하고 있다), 술어 인자가 비-const 참조라는 군더더기까지 있다.
- **제안**: 위처럼 `CreateUFunction` 과 range-for 로 교체한다. (`ShowConfirmation` 의 `TFunctionRef` 람다는 CommonUI API 가 요구하므로 대상이 아니다.)
- **확신도**: 중간

### 6. 🟢 디자인타임 `GetIcon` 이 호출마다 프로젝트의 모든 IMC 를 조회·동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29-36`
- **범주**: 성능/안전
- **문제**: 위젯 디자이너에서 `GetIcon` 은 갱신마다 불리는데, 그때마다 AssetRegistry 로 `UInputMappingContext` 전부를 조회하고 `FAssetData::GetAsset()` 으로 하나씩 동기 로드하며 선형 탐색한다. 에디터 전용 경로라 런타임 영향은 없지만, IMC 가 늘수록 디자이너가 끊긴다.
- **제안**: 액션+입력 타입을 키로 첫 해석 결과를 `mutable` 캐시에 담거나, 조회 대상을 프로젝트 설정에 등록된 IMC 로 좁힌다.
- **확신도**: 높음

### 7. 🟢 확인 팝업 서술자 팩토리 4개가 같은 코드를 네 벌로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:10-79`
- **범주**: 중복/복잡도
- **문제**: `CreateConfirmationOk`/`OkCancel`/`YesNo`/`YesNoCancel` 은 "서술자 생성 → Header·Body 대입 → 결과별 `FWxConfirmationPopupAction` 을 순서대로 Add" 를 네 번 복제한다. 실제로 다른 것은 넣는 결과 목록뿐이라, 서술자 구성 방식이 바뀌면 네 곳을 함께 고쳐야 한다.
- **제안**: 결과 배열을 받는 내부 함수 하나로 모으고 네 팩토리는 그 목록만 넘긴다. `UWxUILibrary::ShowConfirmationPopup` 의 `EWxPopupButtonLayout` 스위치도 그 목록 선택으로 자연스럽게 줄어든다.
- **확신도**: 높음

### 8. 🟢 `UWxViewModel_AbilitySystem::Deinitialize` 주석이 실제 도달 경로와 어긋난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:64`, 반례 `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:36`
- **범주**: 버그/정확성
- **문제**: "여기는 BeginDestroy 에서만 도달하고, 그때 각 자식도 자기 BeginDestroy 로 구독·티커를 정리한다"고 적혀 있으나 `Initialize` 도 진입부에서 `Deinitialize()` 를 부른다. 지금은 `GetOrCreate` 가 갓 만든 객체에만 `Initialize` 를 태워 배열이 비어 있으므로 무해하지만, 자식 VM 을 "떼기만 하고 죽이지 않는" 판단의 근거로 적힌 주석이라 재초기화 경로가 생기면 그대로 오판을 부른다(자식 구독·티커가 정리되지 않은 채 버려진다).
- **제안**: 주석을 실제 경로대로 고치거나, 재초기화가 설계상 없다면 `Initialize` 진입부의 `Deinitialize()` 호출을 빼 주석이 사실이 되게 한다.
- **확신도**: 중간(현재 동작에는 영향이 없는 문서-코드 불일치다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp` (및 각 대응 헤더)
- **훑은 파일**: `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`, `Config/DefaultGame.ini`(UI 설정 섹션)
- **미검토 / 한계**: BP/WBP 에셋(레이아웃·HUD·팝업·탭 구성, MVVM 바인딩 표)은 범위 밖이라 발견 1·3 의 실제 사용처는 확인하지 못했다. 빌드·PIE 재현은 하지 않았다. 규칙 전수 확인 결과 파일 첫 줄 저작권 누락 0건, `FORCEINLINE`·인라인 정의 0건(`FWxStateTreeTask_*` 헤더의 `GetInstanceDataType()` 은 사유가 적힌 규칙 6 예외), 플러그인 의존 위반 0건이다. 멀티 로컬 플레이어(분할 화면)는 범위 밖으로 보고 `PrimaryGameLayout` 단일 보관, `RefreshGamePause` 의 `NM_Standalone` 게이트, 네임플레이트의 `GetFirstPlayerController`, 인디케이터 매니저를 0번 컨트롤러에서 찾는 것은 문제로 다루지 않았다. `UWxViewModel_Attribute` 가 최대치 미지정 시 현재 어트리뷰트를 최대치로 되쓰는 동작은 `WxMVVMConversionLibrary.h` 에 명시된 의도라 제외했고, 뷰모델을 Outer 로 공유하는 방식·위젯 정지 재평가·`Popup` 명명은 확정된 설계라 다루지 않았다.

---
*문서 기준 커밋 `d24cb681` · 리뷰일 2026-08-28 · 소스 63파일 — `/module-review`로 갱신*
