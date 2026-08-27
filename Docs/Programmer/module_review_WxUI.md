# WxUI — 코드 리뷰

> 건강한 모듈이다. 뷰모델마다 `Deinitialize` 에서 자기 구독을 대칭으로 끊고, 이미지 스트리밍 재진입·CommonUI 위젯 풀 재사용 같은 함정을 주석으로 짚어 가며 막았으며 모듈 경계도 깨끗하다(`WxUI.Build.cs` 의 Wx 의존은 `WxCore` 하나, 소스의 Wx 크로스 include 는 `WxGameplayTags.h`·`WxLocatorUtils.h` 뿐). 이번 리뷰는 63개 소스를 모두 훑고 서브시스템·레이아웃 수명주기, HUD/네임플레이트 컴포넌트, VM 계층 전체(구독/해제 1:1 대조), 인디케이터 투영·등록, StateTree 노드까지 cpp 로 내려가 재검증했고, 판정이 엔진 동작에 걸린 항목은 UE 5.8 엔진 소스(`GameplayEffect.cpp`·`CommonActivatableWidgetContainer.cpp`·`UserWidgetPool.h`·`UObjectHash.h`)로 직접 확인했다. 심각 등급 발견은 이번에도 없다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 9 |

## 결과

### 1. 🟡 확인 팝업이 버튼 외 경로로 닫히면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:81-95`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:93`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:215-219`
- **범주**: 버그/정확성
- **문제**: 결과 전달은 `HandleResultChosen` 한 곳뿐이고, 그 진입점은 버튼 `OnClicked`(13-24행)과 `KillPopup` 두 가지다. 그런데 `KillPopup` 은 저장소 어디에서도 호출되지 않고(선언·정의·`Super` 호출이 전부, BP 노출도 없음) `EWxPopupResult::Killed` 는 도달 불가 값이다. 팝업이 `DeactivateWidgetsInLayer` 의 `Stack->ClearWidgets()`, 컨트롤러 교체 시 레이아웃 `RemoveFromParent`, CommonUI back 입력, BP 의 직접 `DeactivateWidget` 으로 닫히면 `OnResultCallback` 은 바운드된 채 버려지고 호출자는 아무 결과도 받지 못한다. 게다가 엔진 위젯 풀이 인스턴스를 살려 두므로(`FUserWidgetPool::InactiveWidgets` 가 강참조) 같은 팝업이 재사용될 때 `SetupPopup` 이 낡은 바인딩을 조용히 덮어써 흔적조차 남지 않는다.
- **제안**: `UWxConfirmationPopup` 이 `NativeOnDeactivated` 를 오버라이드해 `OnResultCallback.IsBound()` 면 `HandleResultChosen(EWxPopupResult::Killed)` 를 태운다. 정상 경로는 이미 언바인딩 후 `DeactivateWidget` 을 부르므로 중복 발화하지 않고, 그러면 `KillPopup` 은 `DeactivateWidget()` 한 줄로 줄거나 사라진다.
- **확신도**: 높음(코드 경로는 확실하다. 다만 `ShowConfirmation`/`ShowConfirmationPopup` 의 C++ 호출자가 아직 없어 실피해는 잠재 상태다)

### 2. 🟡 스택형·무한 지속 GE 의 스택 수와 지속시간이 UI 에 갱신되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:33-52`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:145-179`
- **범주**: 버그/정확성
- **문제**: `StackCount` 는 초기화 때 한 번 읽고 이후엔 티커 폴링(`UpdateEffectState`)으로만 갱신되는데, 티커는 `CachedDuration > 0.f` 일 때만 등록된다(34·49행). 무한 지속 GE 의 `GetDuration()` 은 `FGameplayEffectConstants::INFINITE_DURATION`(= -1) 이라 이 게이트를 통과하지 못한다. 스택이 쌓일 때 GAS 가 `OnActiveGameplayEffectAddedDelegateToSelf` 를 다시 쏘지도 않는다 — 엔진 `FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec` 은 기존 스택 분기에서 `OnStackCountChange` 만 부르고 `InternalOnActiveGameplayEffectAdded` 를 건너뛴다. 결국 "무한 지속 + 스택형 + `UWxEffectComponent_UIData`" GE 는 첫 스택 이후 표시가 굳는다. 같은 뿌리로 `CachedDuration` 도 1회만 읽으므로, 스택 재적용이 지속시간을 새로 고치는 GE 에서는 `TimeRemaining`·`TimeRemainingPercent` 가 낡은 분모로 계산된다.
- **제안**: `Initialize` 에서 `ASC->OnGameplayEffectStackChangeDelegate(BoundHandle)`(필요하면 `OnGameplayEffectTimeChangeDelegate` 도) 를 구독해 `HandleStackCountChanged` 에서 `SetStackCount` 와 `CachedDuration` 을 함께 갱신하고(`Deinitialize` 에서 해제), 티커에는 남은 시간만 맡긴다.
- **확신도**: 중간(무한 지속 스택형 GE 를 UI 에 띄우는 데이터가 아직 없다면 잠재 결함이다)

### 3. 🟡 인디케이터 슬롯이 "등록 순번" 인덱스라 중간 해제 때 표시가 한 칸씩 밀린다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp:78-92`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp:144`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp:155-166`
- **범주**: 설계/구조
- **문제**: 각 인디케이터 위젯은 리졸버가 박아 준 `IndicatorIndex` 로 매니저 배열을 직접 인덱싱한다(`Registered[IndicatorIndex]`). 매니저는 `Indicators.Add` 로 뒤에 붙이고 `Indicators.Remove` 로 지우는데 `TArray::Remove` 는 순서를 지키며 뒤를 당기므로, 인디케이터가 2개 이상일 때 앞의 것이 해제되면 뒤의 것이 앞 슬롯으로 이동한다. 그 순간 0번 위젯은 남의 목표를 가리키고 1번 위젯은 빈 상태가 된다 — 퀘스트 ST 노드가 동시 목표마다 하나씩 등록하는 구조라 실제로 겹칠 수 있는 상황이다.
- **제안**: 순번 대신 안정적인 키를 쓴다. `AddIndicator` 가 슬롯 번호를 발급해 등록증에 담고 뷰모델은 그 번호로 찾거나(빈 슬롯 재사용), 매니저가 슬롯 배열을 직접 소유해 해제 시 그 자리만 비운다.
- **확신도**: 중간(인디케이터를 한 번에 하나만 쓰는 전제라면 의도된 단순화일 수 있다)

### 4. 🟢 HUD 재-push 게이트가 풀에 보관된 인스턴스를 "떠 있음"으로 오독한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp:53-57`
- **범주**: 버그/정확성
- **문제**: 폰 교체 시 중복 push 를 막는 게이트가 `HUDWidget.IsValid()`(약참조 생존 여부)다. 하지만 CommonUI 는 레이어에서 걷힌 위젯을 파괴하지 않는다 — `UCommonActivatableWidgetContainerBase::ReleaseWidget` 이 `GeneratedWidgetsPool.Release(...)` 로 넘기고, `FUserWidgetPool::InactiveWidgets` 는 `TObjectPtr` 강참조다. 즉 `DeactivateWidgetsInLayer(UI.Layer.Game)` 나 HUD 자신의 `DeactivateWidget` 으로 화면에서 걷힌 뒤에도 약참조는 유효해, 이후 폰이 바뀌어도 HUD 가 다시 올라오지 않고 HUD 없는 상태로 굳는다.
- **제안**: 게이트를 "화면에 실제로 걸려 있는가"로 바꾼다 — `HUDWidget.IsValid() && HUDWidget->IsActivated()` 가 아니면 다시 push 한다.
- **확신도**: 중간(현재 Game 레이어를 비우는 C++ 호출자가 없어 실제 발생은 에셋 사용에 달렸다)

### 5. 🟢 `InitializeViewModels` 재호출 시 이전 ASC 구독이 남는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:122-139`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:77-90`
- **범주**: 버그/정확성
- **문제**: 이전 `CachedASC` 의 태그 이벤트를 끊지 않은 채 새 ASC 에 다시 구독하고 `UWxViewModel_Character` 도 새로 만든다. 같은 ASC 로 재호출되면 `RefreshVisibility` 가 태그 변경마다 중복 실행되고, 다른 ASC 면 옛 ASC 의 바인딩이 그 ASC 수명 동안 남는다. 해제 코드가 `EndPlay` 에만 있어 진입 시 정리 경로가 통째로 빠져 있다. 유일한 호출자(`Source/WxGame/Character/WxEnemyCharacter.cpp:40`)가 `BeginPlay` 1회 호출이라 지금은 드러나지 않는다.
- **제안**: 해제 블록을 헬퍼로 묶어 `EndPlay` 와 `InitializeViewModels` 진입부 양쪽에서 부른다.
- **확신도**: 중간

### 6. 🟢 `BlueprintCallable` 사용 규칙 위반 9곳(뷰모델·탭 리스트·버튼)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:41-42`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66-88`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20-21`
- **범주**: 규칙 위반
- **문제**: 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리에만 허용한다(`UWxUILibrary`·`UWxMVVMConversionLibrary`·`UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 는 적법). 남는 위반은 뷰모델의 실행 함수 `UWxViewModel_Ability::TryActivateAbility`, `UWxTabListWidgetBase` 의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`), 위젯 세터 `UWxButtonBase::SetButtonText` 다. 탭 리스트의 `BlueprintCallable, BlueprintPure` 병기는 중복이기도 하다.
- **제안**: `TryActivateAbility` 는 `UWxMVVMConversionLibrary` 같은 BFL 정적 함수로 옮긴다(MVVM 이벤트 바인딩이 뷰모델 함수를 직접 요구해 불가피하다면 그 근거를 주석으로 남기고 예외로 합의). 탭 리스트·버튼은 BP 에서 실제로 쓰는 것만 남기고 조회 함수는 `BlueprintPure` 단독으로 정리한다. `SetButtonText` 는 MVVM 바인딩 수신 세터로 쓰인다면 합의된 예외에 해당한다.
- **확신도**: 중간(Lyra 이식 잔재이며 BP 에서 이미 쓰이고 있을 수 있다)

### 7. 🟢 네이밍 규칙 위반 2건(델리게이트 타입 `Wx` 누락, 티커 콜백 `Handle` 누락)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:90-91`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:49-51`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:64-66`
- **범주**: 규칙 위반
- **문제**: `FOnTabContentCreated`·`FOnTabContentCreatedNative` 만 모듈 내 다른 델리게이트(`FWxPopupResultDelegate`·`FWxOnIndicatorsUpdated`·`FWxOnIndicatorManagerReady`·`FWxPushWidgetToLayerDelegate`)와 달리 `Wx` 접두사가 없다(규칙 1). 또 `FTickerDelegate::CreateUObject` 로 바인딩되는 `UpdateEffectState`·`UpdateCooldownState` 는 델리게이트 콜백인데 `Handle` 접두사가 없다(규칙 4).
- **제안**: 델리게이트는 `FWxOnTabContentCreated`·`FWxOnTabContentCreatedNative` 로 개명한다. 티커 콜백은 `HandleCooldownTick` 류로 바꾸되, `SeedActiveCooldown` 이 직접 부르기도 하는 `UpdateCooldownState` 는 얇은 `Handle...` 래퍼를 두는 편이 호출부를 덜 흔든다.
- **확신도**: 높음(접두사) / 중간(티커를 "델리게이트 콜백"으로 볼지는 해석 여지가 있다)

### 8. 🟢 불필요한 람다 2건
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:24-27`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43-46`
- **범주**: 규칙 위반
- **문제**: 규칙 3(람다는 반드시 필요할 때만). 동적 델리게이트 → 네이티브 델리게이트 브리지는 `FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName())` 로 람다 없이 같은 약참조 의미를 얻는다. `FindByPredicate` 람다도 단순 range-for 로 대체 가능하며(바로 아래 `SetTabHiddenState` 가 같은 탐색을 range-for 로 이미 하고 있다), 현재 술어 인자가 비-const 참조라는 군더더기까지 있다.
- **제안**: 위처럼 `CreateUFunction` 과 range-for 로 교체한다. (`ShowConfirmation` 의 `TFunctionRef` 람다는 CommonUI API 가 요구하므로 대상이 아니다.)
- **확신도**: 중간

### 9. 🟢 디자인타임 `GetIcon` 이 호출마다 프로젝트의 모든 IMC 를 조회·동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29-36`
- **범주**: 성능/안전
- **문제**: 위젯 디자이너에서 `GetIcon` 은 갱신마다 불리는데, 그때마다 AssetRegistry 로 `UInputMappingContext` 전부를 조회하고 `FAssetData::GetAsset()` 으로 동기 로드해 선형 탐색한다. 에디터 전용이지만 IMC 가 늘수록 디자이너가 끊긴다.
- **제안**: 액션+입력 타입을 키로 첫 해석 결과를 `mutable` 캐시에 담거나, 조회 대상을 프로젝트 설정에 등록된 IMC 로 좁힌다.
- **확신도**: 높음

### 10. 🟢 ASC 태그가 바뀔 때마다 어빌리티 VM 전부가 발동 가능 여부를 재평가한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:39`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:369-400`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:41`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:166-182`
- **범주**: 성능/안전
- **문제**: 제네릭 태그 이벤트는 전투 중 빈번히 발화하고(GE 하나에 태그 5개면 5회), 그때마다 어빌리티 VM 수만큼 `GetActivatableAbilities()` 선형 탐색 + `CanActivateAbility`(막히면 `CheckCost` 까지)가 돌며, `RefreshOwnedTags` 는 소유 태그 컨테이너를 통째로 복사·비교한다. 배타 그룹 점유 때문에 구독을 태그로 좁힐 수 없다는 39행 주석의 근거 자체는 타당하다.
- **제안**: 태그 이벤트에선 더티 플래그만 세우고, 이미 존재하는 티커 경로에서 프레임당 1회만 `RefreshActivationState`·`RefreshOwnedTags` 를 수행하도록 합친다.
- **확신도**: 중간(HUD 슬롯 수가 적으면 실측 비용은 미미할 수 있다)

### 11. 🟢 인디케이터 ST 노드가 해석에 성공한 뒤에도 매 틱 로케이터를 다시 푼다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:39-59`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:105-111`
- **범주**: 성능/안전
- **문제**: `RefreshIndicator` 는 조기 반환(등록증의 대상 컴포넌트가 그대로면 그냥 나감) *이전에* `Locator.SyncFind(Owner)` 를 먼저 호출한다. 즉 이미 대상을 잡아 안정된 상태에서도 매 프레임 UOL 해석(경로 기반 조회)이 돈다. 완료 없이 상태가 유지되는 동안 계속 도는 태스크라 비용이 그대로 프레임에 쌓인다.
- **제안**: 재해석 주기를 늦춘다 — 인스턴스 데이터에 누적 시간을 두고 N초에 한 번만(또는 등록증이 아직 컴포넌트를 못 잡은 동안에만) `SyncFind` 를 태운다. 스트리밍 인/아웃 추종은 이 정도 주기로 충분하다.
- **확신도**: 중간(매 틱 재시도 자체는 주석에 적힌 의도지만, 해석 성공 후까지 도는 것은 의도로 보기 어렵다)

### 12. 🟢 활성 이펙트 VM 생성 로직이 두 벌이다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:133-164`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:184-201`
- **범주**: 중복/복잡도
- **문제**: `BuildActiveEffectViewModels`(초기 스캔)와 `HandleActiveEffectAdded`(증분)가 "`UWxEffectComponent_UIData` 를 찾고 → VM 을 만들고 → 배열에 넣고 → 필드 변경을 알린다"를 각자 구현한다. 한쪽만 고치면 초기 목록과 이후 목록의 규칙이 갈린다. 부수적으로 `HandleActiveEffectAdded` 는 `Initialize` 실패 여부를 보지 않고 배열에 넣는데, 실패하면 `BoundHandle` 이 무효라 제거 통지와 영원히 매칭되지 않는 유령 항목이 남는다.
- **제안**: `AddEffectViewModel(ASC, Handle, UIData)` 하나로 합치고, `Initialize` 가 유효한 핸들을 잡았을 때만 배열에 넣는다.
- **확신도**: 높음(중복) / 낮음(유령 항목은 현재 도달 가능한 입력이 없어 이론상 경로다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp` (및 각 대응 헤더)
- **훑은 파일**: `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`
- **미검토 / 한계**: BP/WBP 에셋(레이아웃·HUD·팝업·탭 구성, MVVM 바인딩 설정)은 범위 밖이라 발견 1·4·6 의 실제 사용처와 발생 여부는 확인하지 못했다. 빌드·PIE 재현은 하지 않았고, 엔진 동작에 걸린 판정(발견 1·2·4)은 UE 5.8 설치본 소스를 직접 읽어 확인했다. 멀티 로컬 플레이어(분할 화면)는 v1 범위가 아니라고 보고 `PrimaryGameLayout` 단일 보관, `RefreshGamePause` 의 `NM_Standalone` 게이트, 네임플레이트의 `GetFirstPlayerController` 사용, 인디케이터 매니저를 0번 컨트롤러에서 찾는 것은 문제로 다루지 않았다. `UWxViewModel_Attribute` 가 최대치 미지정 요청에서 현재 어트리뷰트를 최대치로 되쓰는 동작(같은 어트리뷰트에 델리게이트가 두 번 걸리고 `AttributePercent` 가 항상 1)은 `WxMVVMConversionLibrary.h:26` 에 명시된 의도라 제외했다. `FWxStateTreeTask_*` 헤더의 `GetInstanceDataType()` 인라인 정의는 코드에 사유가 적힌 규칙 6 예외이며, 모듈 전반의 파일 첫 줄 저작권·`FORCEINLINE` 금지·플러그인 의존 규칙은 전수 확인해 위반이 없었다. cpp 내 익명 namespace 헬퍼(4개 파일)는 `CLAUDE.md` 에 명문 규칙이 없어 발견으로 올리지 않았다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 63파일 — `/module-review`로 갱신*
