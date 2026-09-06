# WxUI — 코드 리뷰

> 여전히 건강한 모듈이다. 비동기 push 취소·재진입 가드, 티커 재등록 게이트, 이미지 요청 슬롯 무효화, 스크린 투영 접힘 보정 같은 까다로운 지점이 의도적으로 방어돼 있고 그 이유가 대부분 주석으로 남아 있다. 이번 리뷰는 `Build.cs`·`uplugin` 의존 경계부터 UI 매니저·비동기 push·뷰모델 9종·인디케이터 투영·네임플레이트·팝업/버튼 위젯·StateTree 노드 2종까지 cpp 본문을 포함해 봤고, 직전 리뷰(`303d8d7f`)의 발견 5건은 하나도 손대지 않아 그대로 남아 있다. 새로 추가한 것은 코딩 규칙 위반 2건과 표시 정책 비대칭·팩토리 중복 각 1건이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 확인 팝업 실패 경로가 조용해 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:66-79`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:309-315`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:84-102`
- **범주**: 버그/정확성
- **문제**: `ShowConfirmation` 은 `SetBeforePushCallback` 만 걸고 `SetCompletionCallback` 은 걸지 않는다. 그래서 push 가 실패하는 모든 경로 — 클래스 미지정(`WxAsyncAction_PushWidgetToLayer.cpp:24-28`), 레이아웃 부재(`:33-37`), 로드 중 레이아웃 교체(`:100-104`), 레이어 태그 오지정(`:127-131`) — 에서 `ResultCallback` 이 한 번도 실행되지 않고 로그도 남지 않는다. 클래스 오지정도 같다: 로드된 위젯이 `UWxGamePopup` 이 아니면 `HandleConfirmationPopupReady` 의 `Cast`(`:311`)가 실패해 `SetupPopup` 이 건너뛰어지고, 버튼이 연결되지 않은 팝업이 뜬 채 콜백도 오지 않는다. `UWxUILibrary::ShowConfirmationPopup` 은 BP 노출 API 라, 결과를 기다려 흐름을 재개하는 BP 는 그대로 멈춘다.
- **제안**: `ShowConfirmation` 에서도 `SetCompletionCallback` 을 걸어, 위젯이 null 이거나 `UWxGamePopup` 캐스트가 실패하면 `ResultCallback` 을 `EWxPopupResult::Killed`(또는 별도 실패 값)로 한 번 실행하고 `LogWxUI` 경고를 남긴다. 발견 5 와 함께 고치면 `Killed` 가 의미를 되찾는다.
- **확신도**: 높음 (다만 지금 `ShowConfirmationPopup` 을 부르는 C++ 은 없고 `ConfirmationPopupClass` 도 지정돼 있어, 당장 터지는 문제가 아니라 첫 사용자가 밟을 잠복 결함이다)

### 2. 🟡 PlayerController 가 교체되면 걸어 둔 게임 정지가 풀리지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:214-243` (해제 누락), `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:142-166` (`RefreshGamePause` 조기 반환)
- **범주**: 설계/구조 (상태 관리)
- **문제**: `HandlePlayerControllerSet` 은 `TrackedPlayerController.Reset()`(220행)을 **레이아웃 철거(224-228행)보다 먼저** 한다. 그래서 철거로 위젯이 비활성화되며 들어오는 통지에도 `RefreshGamePause` 는 `if (!PC) return;`(151-155행)에서 빠져나가고, 나가는 PC 에 `SetPause(false)` 를 부르는 코드는 어디에도 없다. 엔진은 `AGameModeBase::Pausers` 를 `APlayerController::SetPause(false)` 나 `ForceClearUnpauseDelegates(PauseActor)` 로만 비우는데, 후자는 델리게이트의 UObject 가 그 액터일 때만 매칭한다 — 여기 `FCanUnpause`(160행)의 소유자는 PC 가 아니라 서브시스템이므로 옛 PC 가 파괴돼도 항목이 남는다. 결과적으로 `bPauseGame` 위젯이 떠 있는 동안 같은 월드에서 PC 가 교체되면 월드가 정지된 채 굳는다. (맵 이동은 GameMode·WorldSettings 가 새로 만들어져 함께 사라지므로 영향이 없다.)
- **제안**: `TrackedPlayerController.Reset()` 전에 나가는 PC 에 `SetPause(false)` 를 한 번 부른다. 혹은 순서를 뒤집어 레이아웃 철거·`WatchPawnTags(nullptr)` 를 먼저 하고 그 뒤에 추적을 놓아, 철거가 일으키는 비활성화 통지가 정상 해제 경로를 타게 한다.
- **확신도**: 중간 (같은 월드 내 PC 교체 + 정지 위젯 열림이 동시에 성립해야 하는 좁은 조건이다)

### 3. 🟡 메뉴가 떠도 네임플레이트는 계속 떠 있다 — 인디케이터와 정책이 어긋난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31-77`, 대비 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:109-115`
- **범주**: 설계/구조
- **문제**: 둘 다 `EWidgetSpace::Screen` 위젯 컴포넌트라 UI 레이어 스택 밖에서 뷰포트에 직접 붙는 같은 처지인데(`WxNameplateComponent.cpp:17`, `WxIndicator.cpp:34`), 인디케이터만 `UIManager->IsMenuLayerActive()` 로 메뉴·모달이 열린 동안 스스로 물러난다. 네임플레이트의 `TickComponent` 에는 같은 판정이 없어, 인벤토리·메인메뉴·확인 팝업이 열려 게임이 정지된 동안에도 `State.LockedOn`/`State.Engaged` 가 붙은 대상의 네임플레이트가 계속 그려진다(이 태그들은 메뉴를 연다고 걷히지 않는다). 인디케이터 쪽 주석이 밝힌 "메뉴가 덮어 주지 못한다"는 근거가 네임플레이트에도 그대로 적용된다.
- **제안**: 네임플레이트의 가시성 판정 앞에 `IsMenuLayerActive()` 를 한 줄 넣거나, 두 컴포넌트가 공유할 "스크린 스페이스 표시 허용" 질의를 UI 매니저에 하나 두고 양쪽이 부른다.
- **확신도**: 중간 (메뉴 위에도 네임플레이트를 남기는 것이 의도일 수 있다 — 다만 같은 모듈 안에서 두 위젯의 정책이 갈리는 것은 명시된 결정으로 보이지 않는다)

### 4. 🟡 규칙 위반: 뷰모델에 `BlueprintCallable` 을 달았다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:41-42`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수로 한정한다. `UWxViewModel_Ability::TryActivateAbility` 는 둘 다 아니다. 모듈 안에서 이 규칙을 지키는 다른 두 곳(`WxUILibrary.h:38,41,44` = BP 함수 라이브러리, `WxAsyncAction_PushWidgetToLayer.h:29` = 비동기 액션 팩토리)과 대비된다.
- **제안**: `UWxUILibrary` 에 `TryActivateAbility(UWxViewModel_Ability*)` 형태의 정적 진입점을 두고 뷰모델의 `UFUNCTION` 은 걷는다. BP 에서 부를 필요가 없다면 지정자만 제거한다.
- **확신도**: 높음

### 5. 🟢 `UWxGamePopup::KillPopup` 은 호출자가 없는 데드 코드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxConfirmationPopup.h:20`
- **범주**: 중복/복잡도
- **문제**: `KillPopup` 은 저장소 전체에서 정의·`Super::` 호출 외에 호출자가 없고 `UFUNCTION` 이 아니라 BP 에서도 부를 수 없다. 그 결과 `EWxPopupResult::Killed`(`WxGamePopup.h:15`)도 발생하지 않는 값으로 남는다. 부수적으로 버튼 3종 외의 경로로 팝업이 닫히면(`UWxUILibrary::DeactivateWidgetsInLayer` 의 `ClearWidgets`, CommonUI back 액션 등) 결과 콜백이 유실되는데, 이 구멍을 메우라고 만든 것이 바로 `KillPopup` 이다. 같은 파일의 `FWxConfirmationPopupAction::operator==`(`WxGamePopup.h:35`, 정의 `WxGamePopup.cpp:5-8`)도 사용처가 없다.
- **제안**: `UWxConfirmationPopup::NativeOnDeactivated` 에서 아직 콜백이 남아 있으면 `KillPopup` 을 태우도록 이어 붙이거나(그러면 `Killed` 가 의미를 되찾는다), 쓰지 않기로 하면 `KillPopup`·`Killed`·`operator==` 를 함께 걷어낸다.
- **확신도**: 높음

### 6. 🟢 `UWxViewModel_Effect::UpdateEffectState` 는 티커를 멈추면서 핸들을 비우지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:182,191,201`
- **범주**: 버그/정확성
- **문제**: 형제 격인 `UWxViewModel_Ability::UpdateCooldownState` 는 `false` 를 반환하는 자리마다 `TickerHandle.Reset()` 을 하고 그 이유를 주석으로 남겼다(`WxViewModel_Ability.cpp:403-406,421`). 이펙트 쪽은 세 곳 모두 핸들을 남긴 채 `false` 를 반환해, 이후 `TickerHandle.IsValid()` 가 "돌고 있는 티커가 있다"는 뜻이 아니게 되고 `Deinitialize`(`:64-68`)는 이미 제거된 티커에 `RemoveTicker` 를 건다(핸들이 재사용되지 않아 오작동은 아니다). 201행(`!World`)은 실질 영향도 있다 — 한 번 걸리면 티커가 영구히 멈춰 남은 시간 표시가 언다.
- **제안**: 세 반환 지점에서 `TickerHandle.Reset()` 을 함께 한다. 201행은 반환 대신 이번 프레임만 건너뛰고 `true` 를 돌려주는 편이 의미에 맞다.
- **확신도**: 중간

### 7. 🟢 네임플레이트는 뷰모델 주입 실패를 삼킨다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:110`
- **범주**: 버그/정확성 (진단)
- **문제**: `View->SetViewModelByClass(CharacterViewModel)` 의 반환값을 버린다. 뷰모델 소스가 수동 지정(Creation Type: Manual)이 아니면 주입이 실패하는데, 위젯은 그대로 뜨고 값만 비어 보여 원인을 짚을 단서가 없다. 같은 실패를 `AWxIndicator::BindViewModel` 은 잡아 경고까지 남기고 그 이유를 주석으로 적어 뒀다(`WxIndicator.cpp:97-102`). 같은 함수 안의 다른 실패 경로(ASC 없음 `:82-86`, MVVM View 확장 없음 `:101-106`)는 이미 경고를 남기고 있어 여기만 비어 있다.
- **제안**: `WxIndicator.cpp:97-102` 와 같은 형태로 반환값을 확인해 실패 시 `LogWxUI` 경고를 남긴다.
- **확신도**: 중간 (의도적으로 생략한 것일 수 있으나, 같은 모듈·같은 함수의 다른 경로와 어긋난다)

### 8. 🟢 규칙 위반: 티커 델리게이트에 바인딩되는 콜백에 `Handle` prefix 가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:144,146`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:72-73`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h:82`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `FTickerDelegate::CreateUObject` 로 바인딩되는 `UpdateCooldownState`(`WxViewModel_Ability.cpp:38-40`), `FlushActivationRefresh`(`:386-388`), `FlushOwnedTagsRefresh`·`FlushAbilityRebind`(`WxViewModel_AbilitySystem.cpp:220-222,233-235`), `UpdateEffectState`(`WxViewModel_Effect.cpp:56-58`) 5개가 모두 prefix 없이 있다. 같은 모듈의 다른 델리게이트 콜백(`HandleImageLoaded`, `HandleTagChanged`, `HandleResultChosen`, `HandleObservedWidgetActivationChanged` 등)은 규칙을 지키고 있어, 티커 계열만 규칙 밖에 있는 셈이다.
- **제안**: `HandleCooldownTick`·`HandleActivationRefreshTick` 처럼 이름을 맞춘다. `UpdateCooldownState` 는 `RefreshBoundAbility`(`WxViewModel_Ability.cpp:215`)에서 직접도 호출하므로, 티커용 얇은 `Handle...` 래퍼를 두고 본체 이름을 유지하는 쪽이 깔끔하다.
- **확신도**: 높음

### 9. 🟢 팝업 서술자 팩토리 4종이 버튼 목록만 다른 복사본이다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:10-79`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:92-99`
- **범주**: 중복/복잡도
- **문제**: `CreateConfirmationOk`/`OkCancel`/`YesNo`/`YesNoCancel` 4개가 `NewObject` → Header/Body 대입 → `FWxConfirmationPopupAction` 을 1~3개 `Add` 하는 동일 골격의 복사본이라 70행을 쓴다. 유일한 호출부(`WxUILibrary.cpp:92-99`)는 `EWxPopupButtonLayout` 을 switch 로 다시 이 4개에 매핑하므로, 결국 같은 정보가 열거형과 함수 이름 양쪽에 이중으로 표현돼 있다. 버튼 조합을 하나 늘리면 두 곳을 함께 고쳐야 한다.
- **제안**: `UWxGamePopupDescriptor::CreateConfirmation(EWxPopupButtonLayout, Header, Body)` 하나로 합치고 조합→결과 배열 매핑을 그 안에 둔다. 4개 정적 함수는 필요하면 얇은 위임으로 남긴다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/Tests/WxItemViewModelTests.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Public/` 전 헤더
- **확인만 하고 문제를 못 찾은 지점(기록용)**:
  - 의존 경계는 깨끗하다 — `WxUI.Build.cs` 의 Wx 의존은 `WxCore` 하나뿐이고 `uplugin` 도 같다. cpp/헤더가 include 하는 Wx 헤더(`WxGameplayTags.h`·`WxUIData.h`·`WxLocatorUtils.h`)도 전부 `Plugins/WxCore/Source/WxCore/Public/` 소속이다.
  - 소스 57파일 전부의 첫 줄이 `// Copyright Woogle. All Rights Reserved.` 다(규칙 2 충족).
  - 헤더 인라인 정의는 StateTree 두 노드의 `GetInstanceDataType()` 뿐이고, 두 헤더 모두 예외 사유 주석을 달고 있다(규칙 6 충족).
  - `UWxViewModel::FindSharedViewModel` 의 `GetObjectsWithOuter(..., EGetObjectsFlags::None, RF_NoFlags, EInternalObjectFlags::Garbage)` 는 "중첩 제외 + 가비지 제외" 의도대로 동작한다.
  - 사망 화면을 태그 제거 시 걷지 않는 것(`WxUIManagerSubsystem.cpp:279-284`, 대화는 걷는다 `:292-298`)을 의심해 봤으나, 직전 리뷰가 `UWxAbility_Death` 미종료·플레이어 부활 경로 부재를 확인해 뒀고 이번에도 상황이 같아 발견으로 올리지 않았다.
  - `UWxActivatableWidget::GetDesiredInputConfig` 가 `Super::` 를 부르지 않는 것은 값 반환 가상 함수의 전면 대체라 정상이다. StateTree 두 노드의 `GetDescription`·`PostEditInstanceDataChangeChainProperty` 도 같다.
  - `PushWidgetInstanceToLayerStack` 은 `UFUNCTION` 이 아니라, BP 가 정지 관찰(`ObserveWidgetForGamePause`)을 우회해 위젯을 밀어 넣을 경로는 없다.
  - `UWxAsyncAction_PushWidgetToLayer` 의 동기 완료 재진입(`Activate` 45-59행, `HandleWidgetClassLoaded` 120-144행)은 `bFinished` 게이트로 모든 분기가 막혀 있다.
  - `UWxViewModel_Item::Deinitialize`(`:28-42`)만 `Super::Deinitialize()` 를 먼저 부른다(베이스 문서 `WxViewModel.h:40` 은 "자기 정리 후 Super"). 여기서는 순서가 결과를 바꾸지 않아 발견으로 올리지 않았다.
- **미검토 / 한계**: WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이다. `AWxIndicator::UpdateProjection` 의 화면 밖 접힘·역투영 수식은 로직을 따라 읽었으나 실행 검증은 하지 않았다. 성능은 코드 판독 기준이며 프로파일링하지 않았다(네임플레이트의 프레임당 `UGameplayStatics::GetPlayerPawn`, 인디케이터의 프레임당 행렬 역변환은 규모상 문제로 보지 않아 발견에 넣지 않았다). 발견 3 은 두 위젯이 실제로 어느 z-order 로 뷰포트에 붙는지까지는 확인하지 않고, 코드상 표시 정책이 갈리는 사실만 근거로 삼았다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 57파일 — `/module-review`로 갱신*
