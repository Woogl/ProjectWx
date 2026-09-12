# WxUI — 코드 리뷰

> 레이어 스택·비동기 push·뷰모델 수명 관리는 전반적으로 단단하고 함정마다 주석이 붙어 있으나, 팝업 결과 콜백이 유실되는 경로가 하나 남아 있고 이것이 프론트엔드를 잠근다. README로 진입점을 잡고 UI 매니저·비동기 push·팝업·어빌리티/이펙트/캐릭터 뷰모델·인디케이터·네임플레이트의 cpp까지 내려가 검토했으며, 팝업 결과와 정지 해제는 소비자(WxGame)까지 따라가 영향을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 확인 팝업 결과 콜백 유실이 프론트엔드를 영구 잠금 상태로 만든다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:74`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:24`
- **범주**: 버그/정확성
- **문제**: `ShowConfirmation`은 `SetBeforePushCallback`만 걸고 `SetCompletionCallback`은 걸지 않는다. 그래서 `WxAsyncAction_PushWidgetToLayer` 안의 실패 `Finish(nullptr)` 경로(클래스 미지정·스트리밍 실패·레이아웃 부재·`CreateWidget` 실패·레이어 push 실패, 각각 `:24`·`:33`·`:56`·`:87`·`:94`·`:109`·`:116`·`:129`)에서 `ResultCallback`은 실행되지 않고 `Finish`가 그대로 버린다. 버튼을 거치지 않은 강제 종료도 같다 — `UWxUILibrary::DeactivateWidgetsInLayer`(`Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:67`)로 Modal 레이어를 비우거나 `DeactivateOwningActivatable`로 닫으면 `HandleResultChosen`이 돌지 않는다. 이 경우를 위해 만든 `KillPopup`/`EWxPopupResult::Killed`는 저장소 전체에 호출자가 하나도 없다.
  실제 소비자에서 이것이 소프트락이 된다. `UWxFrontEndWidget::HandleSelectDestination`(`Source/WxGame/FrontEnd/WxFrontEndWidget.cpp:91`)은 팝업을 띄우기 **전에** `SelectDestination`으로 단계를 `Confirmation`으로 올리고, `UWxGameFlowSubsystem::IsFrontEndInputBlocked`(`Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp:185`)가 그 단계에서 true를 반환해 프론트엔드의 모든 버튼을 비활성화한다. 이 단계를 벗어나는 유일한 경로가 팝업 결과로 호출되는 `ResolveStartConfirmation`이므로, 결과 한 번을 잃으면 화면에 팝업도 없고 버튼도 전부 죽은 상태로 고정된다.
- **제안**: `ShowConfirmation`에서 `SetCompletionCallback`을 함께 걸어 push 실패(`Widget == nullptr`)를 `Killed`로 흘려보낸다. 팝업 쪽에서는 버튼을 거치지 않은 비활성화(`NativeOnDeactivated` 등)에서 `KillPopup`을 태워 미처리 콜백을 1회 완료시킨다 — 중복은 이미 `HandleResultChosen`의 언바인딩이 막는다.
- **확신도**: 높음

### 2. 🟡 사망 화면 push에는 대화 화면과 달리 취소·재확인 가드가 없다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:277`
- **범주**: 버그/정확성
- **문제**: `HandleDeathTagChanged`는 `PushWidgetToLayer` 결과를 로컬 변수에 받고 곧바로 `Activate()`만 한다. 진행 중인 요청을 기억하지 않으므로 취소할 수단이 없고, 완료 시점에 사망 태그가 아직 붙어 있는지 재확인하지도 않는다. 바로 아래 대화 경로(`:292`~`:296`, `:315`~`:320`)는 같은 위험을 알고 `PendingDialogueScreenPush` 보관 + 취소 + 완료 시 태그 재확인으로 세 겹을 막아 두었다. `DeathScreenClass`는 소프트 클래스라 세션 첫 사망에는 스트리밍 지연이 있고, 그 창 안에서 사망 태그가 걷히면(치유·치트·부활) 살아 있는 플레이어 위로 사망 화면이 뒤늦게 뜬다. 사망 화면을 닫는 유일한 경로가 화면 자신을 인자로 받는 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:19`)이라, 그 화면이 Menu 레이어에 남으면 걷을 주체가 없다.
- **제안**: 대화 경로와 같은 모양으로 맞춘다 — 진행 중인 push를 멤버로 들고 사망 태그가 걷힐 때(`NewCount <= 0`) 취소하며, 완료 콜백에서 태그가 아직 살아 있는지 한 번 더 보고 아니면 즉시 비활성화한다.
- **확신도**: 중간

### 3. 🟡 컨트롤러 교체 후 정지를 재평가하지 않아 정지가 남을 수 있다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:204`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:141`
- **범주**: 설계/구조
- **문제**: `HandlePlayerControllerSet`은 `TrackedPlayerController.Reset()`(`:210`)을 먼저 하고 그 뒤에 레이아웃을 뜯는다. 철거 중 위젯 비활성화로 `RefreshGamePause`가 불려도 `:141`의 `if (!PC)`에서 빠져나가고, 새 PC를 저장한 뒤(`:229`)에도 재평가를 하지 않는다. 프로젝트 게임모드가 `AWxGameMode : public AGameModeBase`(`Source/WxGame/Framework/WxGameMode.h:14`)라 `Pausers`를 매 틱 되묻는 `AGameMode::Tick` 경로가 없으므로, 이렇게 남은 정지는 스스로 풀리지 않고 다음번 `SetPause(false)`가 올 때까지 유지된다. 새 컨트롤러가 `UWxPlayerLayoutComponent::LayoutClass`를 비워 둔 구성(코드가 명시적으로 허용하는 구성)이면 push가 없어 `RefreshGamePause`가 아예 호출되지 않고, 화면에는 아무 UI도 없는데 월드만 멈춘 상태가 된다.
- **제안**: `HandlePlayerControllerSet` 끝, `TrackedPlayerController = PC` 뒤에 `RefreshGamePause()`를 한 번 호출한다(레이아웃이 비어 있으면 `WantsGamePause()`가 false라 해제 요청이 나간다).
- **확신도**: 중간

### 4. 🟡 네임플레이트 하나가 쓰지도 않는 이펙트 뷰모델과 매 프레임 티커를 끌고 온다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:32`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:43`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:67`
- **범주**: 성능/안전
- **문제**: `UWxViewModel_Character::Initialize`가 무조건 `UWxViewModel_AbilitySystem::GetOrCreate`를 호출하고, 컴포지트의 `Initialize`는 ASC 델리게이트 4개를 구독한 뒤 `BuildActiveEffectViewModels`로 아이콘이 있는 활성 GE 전부의 VM을 만든다. 유한 지속 GE마다 `FTSTicker` 코어 티커가 하나씩 붙어 매 프레임 돈다. 이 경로는 적 캐릭터의 네임플레이트 초기화(`Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:109` ← `Source/WxGame/Character/WxEnemyCharacter.cpp:45`)에서도 그대로 타므로, 네임플레이트 WBP가 버프 목록이나 잔여 시간을 바인딩하지 않아도 적 수 × 버프 수만큼 티커와 구독이 누적된다.
- **제안**: 이펙트 VM 생성과 잔여 시간 갱신을 실제 바인딩 요청 시점으로 미루거나(어트리뷰트·어빌리티 VM처럼 `GetOrCreate...` 지연 생성), 최소한 잔여 시간 갱신을 ASC 단위 티커 하나로 묶는다. 착수 전에 동시 네임플레이트 수와 아이콘 GE 수를 늘려 비용을 먼저 재는 편이 좋다.
- **확신도**: 중간

### 5. 🟢 티커에 직접 바인딩되는 콜백 5개에 `Handle` 접두사가 없다

- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:144`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:146`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:72`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:73`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h:82`
- **범주**: 규칙 위반
- **문제**: `UpdateCooldownState`·`FlushActivationRefresh`·`FlushOwnedTagsRefresh`·`FlushAbilityRebind`·`UpdateEffectState`가 모두 `FTickerDelegate::CreateUObject`로 직접 바인딩된다(`WxViewModel_Ability.cpp:41`, `:411`, `WxViewModel_AbilitySystem.cpp:226`, `:239`, `WxViewModel_Effect.cpp:68`). CLAUDE.md 코딩 규칙 4의 Callback 명명 규칙과 어긋나며, 같은 모듈의 다른 델리게이트 콜백은 모두 `Handle` 접두사를 지키고 있어 일관성도 깨진다.
- **제안**: 선언·정의·바인딩을 함께 `Handle` 접두사로 바꾼다. `UpdateCooldownState`는 `RefreshBoundAbility`에서 일반 함수로도 직접 호출되므로(`WxViewModel_Ability.cpp:239`) 개명 시 그 호출부도 같이 본다.
- **확신도**: 높음

### 6. 🟢 뷰모델 베이스의 `Deinitialize` 계약 주석을 파생 6개가 전부 어긴다

- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h:43`
- **범주**: 설계/구조
- **문제**: 베이스는 "표시 필드 변경은 브로드캐스트하지 않는다", "파생은 자기 정리 후 Super 를 호출한다"고 규정한다. 실제로는 `WxViewModel_Ability.cpp:107`·`WxViewModel_AbilitySystem.cpp:76`·`WxViewModel_Attribute.cpp:64`·`WxViewModel_Effect.cpp:95`·`WxViewModel_Character.cpp:52`·`WxViewModel_Item.cpp:31`이 모두 `RF_BeginDestroyed` 가드를 두고 **브로드캐스트한다**(그래야 재초기화 때 이전 표시가 남지 않는다). Super 호출 위치도 중간(Ability/AbilitySystem/Attribute/Effect)·마지막(Character)·처음(Item)으로 제각각이다. 주석을 그대로 믿고 새 VM을 쓰면 재초기화 시 화면에 옛 값이 남는 버그를 그대로 재현하게 된다.
- **제안**: 베이스 주석을 현재 규약("`RF_BeginDestroyed`가 아니면 표시 필드 초기화를 통지한다")으로 정정하고, Super 호출 위치도 한 가지로 못 박는다.
- **확신도**: 높음

### 7. 🟢 네임플레이트의 뷰모델 주입 실패만 조용하다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:110`
- **범주**: 버그/정확성
- **문제**: `View->SetViewModelByClass(CharacterViewModel)`의 반환값을 버린다. 위젯의 뷰모델 소스가 Manual이 아니면 주입이 실패해도 표시만 비고 원인이 드러나지 않는다. 바로 같은 모듈의 `AWxIndicator::BindViewModel`(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:97`)은 똑같은 호출의 실패를 "Creation Type: Manual 인지 확인" 문구까지 붙여 경고한다 — 진단 가드를 새로 늘리는 문제가 아니라 두 자리의 처리가 어긋난 문제다.
- **제안**: 인디케이터 쪽과 같은 형태의 경고를 남기고 실패 시 조기 반환한다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`.
- **훑은 파일**: `Plugins/WxUI/README.md`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Private/System/WxPrimaryGameLayout.cpp`, `Private/Widget/WxHUDLayout.cpp`, `Private/Widget/WxButtonBase.cpp`, `Private/Widget/WxActivatableWidget.cpp`, `Private/MVVM/WxMVVMConversionLibrary.cpp`, `Private/MVVM/WxViewModel_Subtitle.cpp`, `Private/MVVM/WxViewModel_Item.cpp`, `Private/MVVM/WxViewModel_Interaction.cpp`, `Private/MVVM/WxViewModel_Indicator.cpp`, Public 전체 헤더. 영향 확인을 위해 `Source/WxGame/FrontEnd/*`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Config/DefaultGame.ini`도 함께 봤다.
- **규칙 점검 결과**: Copyright 첫 줄은 56개 전 파일 통과(`WxUIManagerSubsystem.h/.cpp`는 BOM이 앞설 뿐 문구는 정상). 의존성은 `WxCore`만이라 플러그인 경계 위반 없음(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`는 모두 WxCore). `FORCEINLINE`·인라인 정의 없음 — StateTree 두 노드의 `GetInstanceDataType()`은 규칙 6 예외이며 사유 주석이 붙어 있다. 람다는 `WxUILibrary.cpp:23`의 동적→네이티브 델리게이트 어댑터 1건뿐이라 필요한 사용으로 보고 제외했다. `UWxViewModel_Ability::TryActivateAbility`의 `BlueprintCallable`은 2026-07-23 승인된 "뷰모델 Command 예외"에 해당해 위반으로 싣지 않았다.
- **기존 발견 재평가**: 이전 문서의 "어빌리티 뷰모델 빈 슬롯 재초기화 시 이전 표시가 남는다"는 현재 `WxViewModel_Ability::Deinitialize`가 `RF_BeginDestroyed`가 아닐 때 표시 필드를 모두 Set으로 비우며 통지하므로(`:107`~`:121`) 해소된 것으로 보고 제외했다. 팝업 결과 유실은 소비자까지 따라가 소프트락임을 확인해 🟡에서 🔴로 올렸다. `WxViewModel_Effect::UpdateEffectState`가 false 반환 시 `TickerHandle`을 비우지 않는 비대칭은 `FTSTicker::RemoveTicker`가 만료 핸들에 안전하고 재등록 게이트도 없어 실질 영향이 없다고 판단해 싣지 않았다.
- **미검토 / 한계**: WBP 계층·MVVM 바인딩 표·에셋 값은 범위 밖이라 보지 않았다(사망 화면의 `bPauseGame` 실제 설정, 확인 팝업 WBP의 닫기 버튼 배선 등은 미확인). `AWxIndicator::UpdateProjection`의 역투영·클램프 수식은 로직만 따라갔을 뿐 수치 검증은 하지 않았다. 빌드·실행·프로파일링은 하지 않았으므로 4번의 비용 규모는 추정이다. 엔진 내부 동작 중 `AGameModeBase`에 매 틱 `Pausers` 재평가가 없다는 점(3번의 근거)은 UE 5.8 소스를 직접 열어 확인하지 않은 통상 지식이므로, 착수 전 한 번 확인하는 편이 좋다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 56파일 — `/module-review`로 갱신*
