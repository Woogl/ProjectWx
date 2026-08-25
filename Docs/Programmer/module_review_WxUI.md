# WxUI — 코드 리뷰

> 건강한 모듈이다. 델리게이트 구독/해제 대칭, 이미지 스트리밍 재진입, CommonUI 위젯 풀 재사용 같은 함정을 주석으로 짚어 가며 방어했고 모듈 경계(`WxCore` 외 Wx 참조 없음)도 지켜진다. 직전 리뷰의 🔴(인디케이터 등록증이 대상 소멸 시 월드 원점으로 튀는 문제)는 커밋 `6a728feb` 에서 약참조+폴백 좌표로 해소돼, 이번에는 심각 등급 발견이 없다. 이번 리뷰는 63개 소스 전부를 훑고 서브시스템·레이아웃 수명주기, HUD/네임플레이트 컴포넌트, VM 계층, 인디케이터, StateTree 노드는 cpp 까지 내려가 읽었으며 UE 5.8 엔진 소스(CommonUI 컨테이너·`FUserWidgetPool`, GAS 스택 적용 경로, `ULocalPlayer::GetPixelPoint`)로 핵심 가정을 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 확인 팝업이 버튼 외 경로로 닫히면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:87-95`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:93`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:215-219`
- **범주**: 버그/정확성
- **문제**: 결과 전달은 `HandleResultChosen` 한 곳에서만 일어나고, 그 진입점은 버튼 `OnClicked` 와 `KillPopup` 뿐이다. 그런데 `KillPopup` 은 저장소 어디에서도 호출되지 않는다(선언·정의·`Super` 호출이 전부). 팝업이 `DeactivateWidgetsInLayer` 의 `Stack->ClearWidgets()`, 컨트롤러 교체 시 레이아웃 `RemoveFromParent`, BP 의 직접 `DeactivateWidget` 으로 닫히면 `OnResultCallback` 은 바운드된 채 버려지고, 결과를 기다리던 호출자는 `Killed` 조차 받지 못해 흐름이 멈춘다. 엔진 위젯 풀이 인스턴스를 살려 두므로 같은 팝업이 재사용될 때 `SetupPopup` 이 낡은 바인딩을 조용히 덮어써 흔적도 남지 않는다.
- **제안**: `UWxConfirmationPopup` 에서 `NativeOnDeactivated` 를 오버라이드해 `OnResultCallback.IsBound()` 이면 `HandleResultChosen(EWxPopupResult::Killed)` 를 태운다. 정상 경로는 이미 언바인딩 후 `DeactivateWidget` 을 부르므로 중복 발화하지 않으며, 그러면 `KillPopup` 은 `DeactivateWidget()` 호출로 단순화되거나 사라진다.
- **확신도**: 높음(코드 경로는 확실하다. 다만 현재 `ShowConfirmationPopup` 의 C++·BP 호출자가 없어 실피해는 아직 없다)

### 2. 🟡 스택형·무한 지속 GE 의 스택 수와 지속시간이 UI 에 갱신되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:33-52`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:145-179`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:184-201`
- **범주**: 버그/정확성
- **문제**: `StackCount` 는 초기화 때 한 번 읽고 이후엔 티커(`UpdateEffectState`)의 폴링으로만 갱신되는데, 티커는 `CachedDuration > 0.f` 일 때만 등록된다. 스택이 쌓일 때 GAS 는 `OnActiveGameplayEffectAddedDelegateToSelf` 를 발화하지 않는다 — 엔진 `FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec` 은 기존 스택 분기에서 `OnStackCountChange` 만 부르고 `InternalOnActiveGameplayEffectAdded` 를 건너뛴다. 결과적으로 무한 지속 + 스택형 + `UWxEffectComponent_UIData` 를 단 GE 는 첫 스택 이후 표시가 굳는다. 같은 뿌리로 `CachedDuration` 도 1회만 읽으므로, 스택 재적용이 지속시간을 새로 고치는 GE 에서는 `TimeRemaining`·`TimeRemainingPercent` 가 낡은 분모로 계산된다.
- **제안**: `Initialize` 에서 `ASC->OnGameplayEffectStackChangeDelegate(Handle)`(필요하면 `OnGameplayEffectTimeChangeDelegate(Handle)` 도)을 구독해 `HandleStackCountChanged` 에서 `SetStackCount` 와 `CachedDuration` 을 함께 갱신하고(`Deinitialize` 에서 해제), 티커에는 남은 시간만 맡긴다.
- **확신도**: 중간(무한 지속 스택형 GE 를 UI 에 띄우는 데이터가 아직 없다면 잠재 결함이다)

### 3. 🟢 `BlueprintCallable` 사용 규칙 위반(뷰모델·탭 리스트)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:41-42`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66-88`
- **범주**: 규칙 위반
- **문제**: 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Async Action 팩토리에만 허용한다. `UWxViewModel_Ability::TryActivateAbility` 는 뷰모델의 실행 함수이고, `UWxTabListWidgetBase` 의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)는 위젯의 MVVM 수신용 1-arg setter 예외에도 해당하지 않는다. `BlueprintCallable, BlueprintPure` 병기도 중복이다.
- **제안**: `TryActivateAbility` 는 `UWxMVVMConversionLibrary` 같은 BFL 정적 함수로 옮긴다. MVVM 이벤트 바인딩이 뷰모델 함수를 직접 요구해 불가피하다면 그 근거를 주석으로 남기고 예외로 합의한다. 탭 리스트는 BP 에서 실제로 쓰는 것만 남기고 `BlueprintPure` 단독으로 정리한다.
- **확신도**: 중간(Lyra 이식 잔재이며 BP 에서 이미 쓰이고 있을 수 있다)

### 4. 🟢 HUD 재-push 게이트가 풀에 보관된 인스턴스를 "떠 있음"으로 오독한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp:53-57`
- **범주**: 버그/정확성
- **문제**: 폰 교체 시 중복 push 를 막는 게이트가 `HUDWidget.IsValid()`(약참조 생존 여부)다. CommonUI 는 레이어에서 걷힌 위젯을 파괴하지 않고 `FUserWidgetPool::Release` 로 `InactiveWidgets` 에 보관하며 그 배열이 `TObjectPtr` 강참조다. 즉 `DeactivateWidgetsInLayer(UI.Layer.Game)` 이나 HUD 자신의 `DeactivateWidget` 으로 화면에서 걷힌 뒤에도 약참조는 유효해, 이후 폰이 바뀌어도 HUD 가 다시 올라오지 않고 HUD 없는 상태로 굳는다.
- **제안**: 게이트를 "화면에 실제로 걸려 있는가"로 바꾼다 — `HUDWidget.IsValid() && HUDWidget->IsActivated()` 로 판정하고 아니면 다시 push 한다.
- **확신도**: 중간(현재 Game 레이어를 비우는 C++ 호출자가 없어 실제 발생은 에셋 사용에 달렸다)

### 5. 🟢 `InitializeViewModels` 재호출 시 이전 ASC 구독이 남는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:122-139`
- **범주**: 버그/정확성
- **문제**: 이전 `CachedASC` 의 태그 이벤트를 끊지 않은 채 새 ASC 에 다시 구독한다. 같은 ASC 로 재호출되면 `RefreshVisibility` 가 태그 변경마다 중복 실행되고, 다른 ASC 면 옛 ASC 의 바인딩이 그 ASC 수명 동안 남는다. 유일한 호출자(`Source/WxGame/Character/WxEnemyCharacter.cpp:43`)가 `BeginPlay` 1회 호출이라 지금은 드러나지 않는다.
- **제안**: 함수 진입에서 `EndPlay` 와 같은 해제 블록을 먼저 수행한다(해제를 헬퍼로 묶어 양쪽에서 호출).
- **확신도**: 중간

### 6. 🟢 네이밍 규칙 위반 2건(델리게이트 타입 `Wx` 누락, 티커 콜백 `Handle` 누락)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:90-91`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:49-51`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:64-66`
- **범주**: 규칙 위반
- **문제**: `FOnTabContentCreated`·`FOnTabContentCreatedNative` 만 모듈 내 다른 델리게이트(`FWxPopupResultDelegate`·`FWxOnIndicatorsUpdated`·`FWxPushWidgetToLayerDelegate`)와 달리 `Wx` 접두사가 없다(규칙 1). 또 `FTickerDelegate::CreateUObject` 로 바인딩되는 `UpdateEffectState`·`UpdateCooldownState` 는 델리게이트 콜백인데 `Handle` 접두사가 없다(규칙 4).
- **제안**: 델리게이트는 `FWxOnTabContentCreated`·`FWxOnTabContentCreatedNative` 로 개명한다. 티커 콜백은 `HandleCooldownTick` 류로 바꾸되, 직접 호출도 하는 `UpdateCooldownState` 는 얇은 `Handle...` 래퍼를 두는 편이 호출부를 덜 흔든다.
- **확신도**: 높음(접두사) / 중간(티커를 "델리게이트 콜백"으로 볼지는 해석 여지가 있다)

### 7. 🟢 불필요한 람다 2건
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:24-27`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43-46`
- **범주**: 규칙 위반
- **문제**: 규칙 3(람다는 반드시 필요할 때만). 동적 델리게이트 → 네이티브 델리게이트 브리지는 `FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName())` 로 람다 없이 같은 약참조 의미를 얻는다. `FindByPredicate` 람다도 단순 range-for 로 대체 가능하며, 현재 술어 인자가 비-const 참조라는 군더더기까지 있다.
- **제안**: 위처럼 `CreateUFunction` 과 range-for 로 교체한다. (`ShowConfirmation` 의 `TFunctionRef` 람다는 CommonUI API 가 요구하므로 대상이 아니다.)
- **확신도**: 중간

### 8. 🟢 디자인타임 `GetIcon` 이 호출마다 프로젝트의 모든 IMC 를 조회·로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29-36`
- **범주**: 성능/안전
- **문제**: 위젯 디자이너에서 `GetIcon` 은 갱신마다 불리는데, 그때마다 AssetRegistry 로 `UInputMappingContext` 전부를 조회하고 `FAssetData::GetAsset()` 으로 동기 로드해 선형 탐색한다. 에디터 전용이지만 IMC 가 늘수록 디자이너가 끊긴다.
- **제안**: 액션+입력 타입을 키로 첫 해석 결과를 `mutable` 캐시에 담거나, 조회 대상을 프로젝트 설정에 등록된 IMC 로 좁힌다.
- **확신도**: 높음

### 9. 🟢 ASC 태그가 바뀔 때마다 어빌리티 VM 전부가 발동 가능 여부를 재평가한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:39`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:369-400`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:41`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:166-182`
- **범주**: 성능/안전
- **문제**: 제네릭 태그 이벤트는 전투 중 빈번히 발화하고, 그때마다 어빌리티 VM 수만큼 `GetActivatableAbilities()` 선형 탐색 + `CanActivateAbility`(막히면 `CheckCost` 까지)가 돌며, `RefreshOwnedTags` 는 소유 태그 컨테이너를 통째로 복사·비교한다. 배타 그룹 점유 때문에 구독을 태그로 좁힐 수 없다는 주석의 근거 자체는 타당하다.
- **제안**: 태그 이벤트에선 더티 플래그만 세우고, 이미 존재하는 티커 경로에서 프레임당 1회만 `RefreshActivationState` 를 수행하도록 합친다.
- **확신도**: 중간(HUD 슬롯 수가 적으면 실측 비용은 미미할 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp` (및 각 대응 헤더)
- **훑은 파일**: `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`
- **미검토 / 한계**: BP/WBP 에셋(레이아웃·HUD·팝업·탭 구성, MVVM 바인딩 설정)은 범위 밖이라 발견 3·4 의 실제 사용처와 발생 여부는 확인하지 못했다. 멀티 로컬 플레이어(분할 화면)는 v1 범위가 아니라고 보고 `PrimaryGameLayout` 단일 보관, `RefreshGamePause` 의 `NM_Standalone` 게이트, 네임플레이트의 `GetFirstPlayerController` 사용은 문제로 다루지 않았다. `FWxStateTreeTask_*` 헤더의 `GetInstanceDataType()` 인라인 정의는 코드에 명시된 규칙 6 예외로 보아 제외했다. 발견 1·2·4 는 엔진 소스로 검증했으나 PIE 재현은 하지 않았다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 63파일 — `/module-review`로 갱신*
