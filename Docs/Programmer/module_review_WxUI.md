# WxUI — 코드 리뷰

> 전반적으로 건강하다. 델리게이트 구독/해제 대칭, 이미지 스트리밍 재진입, 위젯 풀 재사용 같은 함정을 주석으로 짚어 가며 방어한 흔적이 뚜렷하고, 모듈 경계(WxCore 외 Wx 참조 없음)도 지켜진다. 가장 큰 위험은 인디케이터 대상 컴포넌트의 GC 수명 처리 하나다. 이번 리뷰는 61개 소스 파일을 전부 읽었고, 서브시스템/레이아웃 수명주기·VM 계층·인디케이터·StateTree 노드는 cpp까지 깊게 봤으며 엔진 소스(GAS 스택 경로, 레벨 스트리밍 GC, GarbageElimination 기본값)로 핵심 가정을 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🔴 인디케이터 대상이 스트리밍 아웃되면 마커가 월드 원점으로 튄다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorDescriptor.h:55-56`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp:25-31`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:53-58`
- **범주**: 버그/정확성
- **문제**: 등록증의 `TargetComponent`는 강참조 `UPROPERTY() TObjectPtr`다. 대상 액터가 월드 파티션으로 언로드되면 엔진은 GC 직전 콜백(`FLevelStreamingGCHelper::OnPreGarbageCollect`)에서 레벨 패키지 전체를 `MarkAsGarbage` 하고, 같은 프레임에 GC를 강제한다(`UWorld::UpdateLevelStreaming`). 프로젝트는 `gc.GarbageEliminationEnabled`를 덮어쓰지 않아 엔진 기본값(켜짐)이므로 GC가 이 `TargetComponent`를 null로 지운다. 그 결과 (a) `ProjectIndicator`는 null 컴포넌트를 "고정 좌표 등록증"으로 해석해 `WorldLocation`(컴포넌트형 등록증에선 한 번도 세팅되지 않은 `ZeroVector`) + 오프셋을 투영하고 표시는 켜진 채로 남는다 — 마커가 (0,0,WorldZOffset)을 가리킨다. (b) `RefreshIndicator`는 `Indicator->GetTargetComponent()`(null)와 해석 실패한 `TargetComponent`(null)를 같다고 보고 "이미 좌표 대역"이라며 `TargetLocation` 대역으로 갈아끼우지 않는다. `IsValid()` 분기는 "가비지지만 아직 null이 아닌" 순간을 전제하는데, 마킹이 GC 직전 콜백 안에서 일어나므로 ST 틱이 그 순간을 관측할 기회가 사실상 없다.
- **제안**: 등록증에 "컴포넌트 추종형인지"를 명시적으로 기록한다(예: `bool bFollowsComponent`를 컴포넌트 오버로드 `Initialize`에서 세팅, `TargetComponent`는 `TWeakObjectPtr`로). `ProjectIndicator`는 추종형인데 컴포넌트가 유효하지 않으면 `false`, `RefreshIndicator`는 "추종형인데 대상이 null이 됐으면" 해제 후 `TargetLocation` 대역으로 재등록한다. 대역 재등록이 없는 일반 호출자를 위해 최소한 컴포넌트 오버로드에서 `WorldLocation`을 등록 시점 위치로 채워 두는 것도 함께 고려한다.
- **확신도**: 높음

### 2. 🟡 확인 팝업이 외부 경로로 닫히면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:87-95`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:93`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:207`
- **범주**: 버그/정확성
- **문제**: 결과 전달은 `HandleResultChosen`(버튼 클릭·`KillPopup`)에서만 일어나는데, `KillPopup`은 모듈 안팎 어디서도 호출되지 않는다. `DeactivateWidgetsInLayer`의 `Stack->ClearWidgets()`, 월드 이동 시 레이아웃 `RemoveFromParent`(→ `UCommonActivatableWidget::NativeDestruct`가 `DeactivateWidget` 호출), BP의 직접 `DeactivateWidget` 등으로 닫히면 `OnResultCallback`은 바운드된 채 버려지고, 위젯이 풀에서 재사용될 때 `SetupPopup`이 조용히 덮어쓴다. 결과를 기다리던 호출자(특히 `ShowConfirmationPopup`의 BP 델리게이트)는 `Killed`조차 받지 못해 흐름이 멈춘다.
- **제안**: `UWxConfirmationPopup`에서 `NativeOnDeactivated`를 오버라이드해 `OnResultCallback.IsBound()`이면 `HandleResultChosen(EWxPopupResult::Killed)`를 태운다(정상 경로는 이미 언바인딩 후 `DeactivateWidget`을 부르므로 중복되지 않는다). 그러면 `KillPopup`은 `DeactivateWidget()` 호출로 단순화되거나 제거할 수 있다.
- **확신도**: 중간

### 3. 🟡 무한 지속 스택형 GE의 스택 수가 UI에 갱신되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:31-52`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:162`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:39`
- **범주**: 버그/정확성
- **문제**: `StackCount`는 초기화 시 한 번 읽고 이후엔 티커(`UpdateEffectState`)가 폴링하는데, 티커는 `CachedDuration > 0`일 때만 돈다. 스택이 쌓일 때 GAS는 `OnActiveGameplayEffectAddedDelegateToSelf`를 발화하지 않는다(엔진 `FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec`에서 기존 스택 분기는 `OnStackCountChange`만 호출하고, 해당 델리게이트는 비스택 분기의 `InternalOnActiveGameplayEffectAdded` 끝에서만 발화한다). 따라서 무한 지속 + 스택형 + `UWxEffectComponent_UIData`를 단 GE는 2스택 이후가 화면에 반영되지 않고, 지속형 GE도 스택 변화를 매 프레임 폴링으로만 알아낸다.
- **제안**: `UWxViewModel_Effect::Initialize`에서 `ASC->OnGameplayEffectStackChangeDelegate(Handle)`를 구독해 `HandleStackCountChanged`로 `SetStackCount`를 갱신하고(`Deinitialize`에서 해제), 티커는 남은 시간만 맡긴다.
- **확신도**: 중간(무한 스택형 GE를 UI에 띄우는 데이터가 현재 없다면 잠재 결함)

### 4. 🟡 `BlueprintCallable` 사용 규칙 위반(뷰모델·탭 리스트)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:42-43`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66-88`
- **범주**: 규칙 위반
- **문제**: 코딩 규칙 5는 `BlueprintCallable`을 BFL과 Async Action 팩토리에만 허용한다(예외: 위젯 서브클래스의 MVVM 수신용 1-arg setter). `UWxViewModel_Ability::TryActivateAbility`는 뷰모델의 0-arg 실행 함수이고, `UWxTabListWidgetBase`의 7개 함수(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)는 setter가 아니다. `BlueprintCallable, BlueprintPure` 병기도 중복이다.
- **제안**: `TryActivateAbility`는 `UWxUILibrary`(또는 `UWxMVVMConversionLibrary`)의 정적 함수 `TryActivateAbilityViewModel(UWxViewModel_Ability*)`로 옮긴다. 단, MVVM 이벤트 바인딩이 뷰모델 함수를 직접 요구한다면 그 근거를 주석으로 남기고 예외로 합의한다. 탭 리스트 함수는 BP에서 실제로 쓰는 것만 남기고 `BlueprintPure`만 두거나 라이브러리로 이관한다.
- **확신도**: 중간(Lyra 이식·MVVM 이벤트 바인딩 등 의도된 노출일 수 있음)

### 5. 🟢 `InitializeViewModels` 재호출 시 이전 ASC 구독이 남는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:122-136`
- **범주**: 버그/정확성
- **문제**: 이전 `CachedASC`의 태그 이벤트를 끊지 않고 새 ASC에 다시 구독한다. 같은 ASC로 재호출되면 `RefreshVisibility`가 변경마다 중복 실행되고, 다른 ASC면 옛 ASC의 바인딩이 ASC 수명 동안 남는다. 현재 유일한 호출자(`AWxEnemyCharacter::BeginPlay`)는 1회 호출이라 잠재 결함이다.
- **제안**: 함수 진입에서 `EndPlay`와 같은 해제 블록을 먼저 수행한다(헬퍼로 묶어 양쪽에서 호출).
- **확신도**: 중간

### 6. 🟢 티커 델리게이트 콜백에 `Handle` 접두사가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:49-51`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:64-66`
- **범주**: 규칙 위반
- **문제**: `FTickerDelegate::CreateUObject`로 바인딩되는 `UpdateEffectState`·`UpdateCooldownState`는 코딩 규칙 4(델리게이트 콜백은 `Handle` 접두사)에 어긋난다.
- **제안**: `HandleTick`류로 바꾸거나, 직접 호출도 하는 `UpdateCooldownState`는 얇은 `HandleCooldownTick`이 감싸도록 한다.
- **확신도**: 중간(티커를 "델리게이트 콜백"으로 볼지는 해석 여지)

### 7. 🟢 델리게이트 타입에 `Wx` 접두사 누락
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:90-91`
- **범주**: 규칙 위반
- **문제**: `FOnTabContentCreated`·`FOnTabContentCreatedNative`만 모듈 내 다른 델리게이트(`FWxPopupResultDelegate`·`FWxOnIndicatorsUpdated` 등)와 달리 접두사가 없다(Lyra 이식 잔재).
- **제안**: `FWxOnTabContentCreated`·`FWxOnTabContentCreatedNative`로 개명한다.
- **확신도**: 높음

### 8. 🟢 불필요한 람다 2건
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:24-27`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43-46`
- **범주**: 규칙 위반
- **문제**: 동적 델리게이트 → 네이티브 델리게이트 브리지는 `FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName())`로 람다 없이 같은 약참조 의미를 얻는다. `FindByPredicate` 람다는 단순 루프로 대체 가능하고 술어 인자가 비-const 참조다.
- **제안**: 위처럼 `CreateUFunction`과 range-for로 교체한다. (`ShowConfirmation`의 `TFunctionRef` 람다는 CommonUI API가 요구하므로 제외.)
- **확신도**: 중간

### 9. 🟢 디자인타임 `GetIcon`이 호출마다 프로젝트의 모든 IMC를 동기 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29-36`
- **범주**: 성능/안전
- **문제**: 위젯 디자이너에서 `GetIcon`은 페인트/갱신마다 불리는데, 매번 AssetRegistry로 `UInputMappingContext` 전부를 조회하고 `GetAsset()`으로 로드해 선형 탐색한다. 에디터 전용이지만 IMC 수가 늘수록 디자이너가 끊긴다.
- **제안**: 액션별 첫 해석 결과를 `mutable` 캐시(키: 액션+입력 타입)에 두거나, `CommonInputSettings`/프로젝트 설정에 등록된 IMC만 대상으로 좁힌다.
- **확신도**: 높음

### 10. 🟢 ASC 태그가 바뀔 때마다 어빌리티 VM 전부가 `CanActivateAbility`·`CheckCost`를 재평가한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:39`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:306-309`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:389-394`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:166-182`
- **범주**: 성능/안전
- **문제**: 제네릭 태그 이벤트는 전투 중 초당 수십 회 발화할 수 있고, 그때마다 어빌리티 VM 수만큼 `CanActivateAbility`(+실패 시 `CheckCost`의 스펙 생성/평가)와 `RefreshOwnedTags`의 컨테이너 복사·비교가 돈다. 주석대로 배타 그룹 때문에 태그로 좁힐 수 없는 점은 타당하다.
- **제안**: 태그 이벤트에선 더티 플래그만 세우고 이미 있는 티커 경로에서 프레임당 1회 `RefreshActivationState`를 수행하도록 합친다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp` (및 각 헤더)
- **훑은 파일**: `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`
- **미검토 / 한계**: BP/WBP 에셋(레이아웃·HUD·팝업·탭 구성, MVVM 바인딩 설정)은 범위 밖이라 `TryActivateAbility`·탭 리스트 함수의 실제 BP 사용처는 확인하지 못했다. 멀티 로컬 플레이어(분할 화면)는 v1 범위가 아니라고 보고 `PrimaryGameLayout` 단일 보관을 문제로 다루지 않았다. 발견 1·3은 엔진 소스로 검증했으나 실제 PIE 재현은 하지 않았다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 61파일 — `/module-review`로 갱신*
