# WxUI — 코드 리뷰

> 건강한 모듈이다. 비동기 push 재진입 가드(`bFinished`), 이미지 요청 슬롯 무효화, 스크린 투영 접힘 보정, 공유 뷰모델의 Outer 규약처럼 까다로운 지점이 의도적으로 방어돼 있고 근거가 대부분 주석에 남아 있다. 이번 리뷰는 `Build.cs`·`uplugin` 의존 경계부터 UI 매니저·비동기 push·플레이어 레이아웃 컴포넌트·뷰모델 9종·인디케이터 투영·네임플레이트·팝업/버튼 위젯·StateTree 노드 2종까지 cpp 본문을 포함해 봤고, 뷰모델 소비 측 진입점(`Source/WxGame`)까지 한 단계 따라가 실제 생성 비용을 확인했다. 직전 리뷰(`262e4cca`) 이후 이 모듈의 코드 변경은 테스트 파일 제거와 DataTable ContextString 매크로화뿐이라 기존 발견은 대부분 그대로 유효하며, 아래 3번은 이번에 새로 확인한 항목이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 확인 팝업 결과 콜백이 실패·강제 종료 경로에서 통째로 유실된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:66-79`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:24-28`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:87-89`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80-84`
- **범주**: 버그/정확성
- **문제**: `FWxPopupResultDelegate` 가 "반드시 한 번은 온다"는 계약을 지키지 못하는 구멍이 두 갈래로 나 있다.
  1. `ShowConfirmation` 은 `SetBeforePushCallback` 만 걸고 `SetCompletionCallback` 은 걸지 않는다. 그래서 push 가 실패하는 모든 경로 — 클래스 미지정(`WxAsyncAction_PushWidgetToLayer.cpp:24-28`), 레이아웃 부재(`:33-37`), 로드 중 레이아웃 교체(`:100-104`), 위젯 생성 실패(`:113-118`), 레이어 태그 오지정(`:127-131`) — 에서 `ResultCallback` 이 한 번도 실행되지 않고 로그도 남지 않는다. 클래스를 `UWxConfirmationPopup` 이 아닌 다른 `UWxGamePopup` 파생으로 지정한 경우도 같다: `HandleConfirmationPopupReady`(`WxUIManagerSubsystem.cpp:299-305`)의 `Cast` 자체는 `ConfirmationPopupClass` 의 타입 제약(`WxUIDeveloperSettings.h:25`)으로 성립하지만, 베이스 `UWxGamePopup::SetupPopup` 이 빈 본문(`WxGamePopup.cpp:87-89`)이라 넘긴 콜백이 저장되지 않고 그대로 사라진다.
  2. 성공적으로 떴더라도, 버튼 3종 외의 경로로 닫히면(`UWxUILibrary::DeactivateWidgetsInLayer` 의 `ClearWidgets`, CommonUI back 액션, `HandlePlayerControllerSet` 의 레이아웃 철거) `HandleResultChosen` 이 돌지 않아 `OnResultCallback` 이 그대로 사라진다. 이 구멍을 메우라고 만들어 둔 `KillPopup` 은 저장소 전체에서 정의·`Super::` 호출 외에 호출자가 없고 `UFUNCTION` 도 아니라 BP 에서도 부를 수 없는 데드 코드이며, 그 결과 `EWxPopupResult::Killed`(`WxGamePopup.h:15`)도 발생하지 않는 값으로 남아 있다.
  `UWxUILibrary::ShowConfirmationPopup`(`WxUILibrary.cpp:84-101`)은 BP 노출 API 라, 결과를 기다려 흐름을 재개하는 BP 는 그대로 멈춘다.
- **제안**: `ShowConfirmation` 에서도 `SetCompletionCallback` 을 걸어 위젯이 null 이거나 `UWxConfirmationPopup` 이 아니면 `Killed` 로 한 번 실행하고 `LogWxUI` 경고를 남긴다. 그리고 `UWxConfirmationPopup::NativeOnDeactivated` 에서 콜백이 아직 남아 있으면 `KillPopup` 을 태운다 — 그러면 `Killed` 가 의미를 되찾는다. 쓰지 않기로 한다면 `KillPopup`·`Killed`·사용처 없는 `FWxConfirmationPopupAction::operator==`(`WxGamePopup.h:35`, 정의 `WxGamePopup.cpp:5-8`)를 함께 걷어낸다.
- **확신도**: 높음 (다만 지금 `ShowConfirmationPopup` 을 부르는 C++ 은 없어, 당장 터지는 문제가 아니라 첫 사용자가 밟을 잠복 결함이다)

### 2. 🟡 PlayerController 가 교체되면 걸어 둔 게임 정지를 풀 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:204-233` (해제 누락), `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:132-156` (`RefreshGamePause` 조기 반환)
- **범주**: 설계/구조 (상태 관리)
- **문제**: `HandlePlayerControllerSet` 은 `TrackedPlayerController.Reset()`(`:210`)을 **레이아웃 철거(`:214-218`)보다 먼저** 한다. 그래서 철거로 위젯이 비활성화되며 들어오는 통지에도 `RefreshGamePause` 는 `if (!PC) return;`(`:141-145`)에서 빠져나가고, 나가는 PC 에 `SetPause(false)` 를 부르는 코드는 어디에도 없다. 엔진 정리 경로도 이 항목을 짚지 못한다 — `APlayerController::Destroyed` 가 부르는 `AGameModeBase::ForceClearUnpauseDelegates` 는 `CanUnpauseDelegate.GetUObject() == PauseActor` 로만 매칭하는데, 여기 `FCanUnpause`(`:150`)의 소유자는 서브시스템이라 `Pausers` 에 그대로 남는다. 결과는 두 갈래다.
  - 옛 PC 가 파괴되는 통상 경로: `ForceClearUnpauseDelegates` 가 `PauserPlayerState` 를 비워 월드 시간은 풀리지만 `Pausers` 엔트리는 남아, 이후 `AGameModeBase::IsPaused()` 가 계속 true 를 반환한다(엔트리는 다음 `ClearPause()` 가 돌 때까지 사라지지 않고, `ClearPause()` 는 우리 `RefreshGamePause` 의 `SetPause(false)` 로만 촉발된다).
  - 옛 PC 가 살아 있는 채 LocalPlayer 에서만 떨어지는 경로: `PauserPlayerState` 도 남아 월드가 실제로 정지한 채 유지된다. 새 PC 가 위젯을 하나라도 push 하면 `ObserveWidgetForGamePause → RefreshGamePause → SetPause(false)` 로 풀리지만, 아무것도 띄우지 않는 PC(`LayoutClass` 미지정 등)로 갈아타면 화면엔 아무것도 없는데 월드만 정지한 상태가 남는다.
- **제안**: `TrackedPlayerController.Reset()` 앞에서 나가는 PC 에 `SetPause(false)` 를 한 번 부른다. 혹은 순서를 뒤집어 레이아웃 철거·`WatchPawnTags(nullptr)` 를 먼저 하고 그 뒤에 추적을 놓아, 철거가 일으키는 비활성화 통지가 정상 해제 경로를 타게 한다.
- **확신도**: 중간 (같은 월드 내 PC 교체 + 정지 위젯 열림이 겹쳐야 드러난다. 맵 이동은 GameMode 가 새로 만들어져 영향이 없다)

### 3. 🟡 네임플레이트 하나가 적마다 ASC 컴포지트 전체를 즉시 세운다 — ASC 구독 4개와 이펙트별 매 프레임 티커
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:108-110`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:32`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:38-44`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:64-66` (호출부: `Source/WxGame/Character/WxEnemyCharacter.cpp:48`)
- **범주**: 성능/안전
- **문제**: `UWxNameplateComponent::InitializeViewModels` 는 `UWxViewModel_Character::Initialize` 를 부르고, 그 첫 줄이 `UWxViewModel_AbilitySystem::GetOrCreate(InASC)`(`WxViewModel_Character.cpp:32`)다. 어트리뷰트/어빌리티 VM 은 바인딩 요청 시 지연 생성되지만 **ASC 컴포지트 자신과 그 이펙트 VM 은 지연되지 않는다**. 그래서 네임플레이트 위젯이 지정된 적 하나가 스폰될 때마다 무조건 다음이 붙는다.
  - ASC 전역 구독 4개(`WxViewModel_AbilitySystem.cpp:38-41`): `OnActiveGameplayEffectAddedDelegateToSelf`, `OnAnyGameplayEffectRemovedDelegate`, `RegisterGenericGameplayTagEvent`, `AbilitySpecDirtiedCallbacks`.
  - 제네릭 태그 이벤트가 그 적의 태그 변화마다 `FlushOwnedTagsRefresh` → `RefreshOwnedTags` 를 예약해, 태그가 바뀐 프레임마다 `GetOwnedGameplayTags` 컨테이너를 새로 채우고 비교한다.
  - 아이콘을 가진 활성 GE 마다 `UWxViewModel_Effect` 가 생기고(`WxViewModel_AbilitySystem.cpp:184`), 유한 지속이면 각각 `FTSTicker` 를 지연 없이 등록해 **표시하는 위젯이 없어도 매 프레임 돈다**(`WxViewModel_Effect.cpp:64-66`). 코어 티커라 정지 메뉴 중에도 계속 돈다.
  `AWxEnemyCharacter::BeginPlay`(`Source/WxGame/Character/WxEnemyCharacter.cpp:48`)가 예외 없이 이것을 부르므로, 오픈월드에서 동시 존재 적 수에 비례해 이 비용이 그대로 쌓인다. 네임플레이트 WBP 가 버프 목록이나 어트리뷰트를 실제로 바인딩하는지와 무관하다.
- **제안**: `UWxViewModel_Character` 의 `AbilitySystem` 필드를 지연 해석으로 바꾼다 — 예를 들어 `Initialize` 는 ASC 만 약참조로 들고, 바인딩(또는 `UWxMVVMConversionLibrary::GetAttributeViewModel` 같은 컨버전 함수)이 처음 요청할 때 `GetOrCreate` 를 태운다. 이펙트 VM 의 티커도 잔여 시간 필드를 실제로 읽는 뷰가 붙었을 때만 돌게 하거나, 개별 티커 대신 ASC 당 하나로 묶는 편이 규모에 맞다.
- **확신도**: 중간 (구축 경로와 티커 등록은 코드로 확정되나 실제 프레임 비용은 프로파일링하지 않았고, 아이콘 GE 수는 에셋 쪽 값이라 확인하지 못했다. 컴포지트를 통째로 미리 세우는 것이 의도된 단순화일 수도 있다)

### 4. 🟡 `Deinitialize` 의 통지 계약이 뷰모델마다 갈리고, 어빌리티 VM 은 무통지 대입으로 표시가 어긋날 수 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h:39-45` (베이스 계약), `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:104-117`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:51-57`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp:28-42`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:70-92`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp:43-63`
- **범주**: 설계/구조
- **문제**: 베이스는 `Deinitialize` 를 "파생은 자기 정리 후 Super 를 호출한다"(`WxViewModel.h:40`), "표시 필드 변경은 브로드캐스트하지 않는다"(`:43`)로 못박아 뒀는데, 파생이 서로 다른 세 가지 정책으로 갈려 있다.
  - 무통지 대입: `UWxViewModel_Ability`(`:105-117`), `UWxViewModel_AbilitySystem`(`WxViewModel_AbilitySystem.cpp:71-75`)
  - 파괴 중이 아니면 브로드캐스트: `UWxViewModel_Character`(`:51-57`), `UWxViewModel_Item`(`:39-41`) — 베이스 문구와 정면으로 어긋난다
  - 표시 필드를 아예 건드리지 않음: `UWxViewModel_Effect`(Title·Description·Icon·StackCount 가 그대로 남는다), `UWxViewModel_Attribute`(수치 5종이 그대로 남는다)
  어빌리티 VM 쪽에는 실질 결과도 따라온다. `Initialize` 첫 줄이 `Deinitialize()`(`WxViewModel_Ability.cpp:17`)라 재초기화 시 필드가 통지 없이 비워지는데, 이어지는 `RefreshBoundAbility` 는 매칭 어빌리티가 없으면 `MatchedAbility == CachedAbility.Get()`(둘 다 null)로 조기 반환(`:159-162`)한다. 결과적으로 VM 필드는 비었는데 화면엔 옛 스킬 이름·아이콘이 남는다. 현재 `Initialize` 를 두 번 부르는 경로는 없어 도달하지 않지만, 이 자리의 주석(`:104`)이 그 상황을 정확히 짚어 놓고도 방어는 하지 않은 상태다. 부수적으로 `UWxViewModel_Item::Deinitialize` 만 `Super::Deinitialize()` 를 **먼저** 부른다(`:30`) — 베이스 문구가 요구하는 순서의 반대다.
- **제안**: 베이스 doc 을 실제 정책 하나로 확정한 뒤 파생 전부를 거기에 맞춘다. "파괴 중이 아니면 통지"로 통일하는 편이 재초기화 시나리오까지 자연히 덮는다 — 그러면 어빌리티 VM 은 `RF_BeginDestroyed` 가드를 붙이고, Effect·Attribute VM 은 표시 필드 비우기를 추가하고, Item VM 은 `Super` 호출을 마지막으로 옮기면 된다.
- **확신도**: 중간 (정책이 갈려 있다는 사실과 계약 문구와의 불일치는 확실하다. 어빌리티 VM 의 stale 표시는 현재 도달 경로가 없어 잠복 상태다)

### 5. 🟡 메뉴가 떠도 네임플레이트는 계속 떠 있다 — 인디케이터와 정책이 어긋난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31-77`, 대비 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:109-115`
- **범주**: 설계/구조
- **문제**: 둘 다 `EWidgetSpace::Screen` 위젯 컴포넌트라 UI 레이어 스택 밖에서 뷰포트에 직접 붙는 같은 처지인데(`WxNameplateComponent.cpp:17`, `WxIndicator.cpp:34`), 인디케이터만 `UIManager->IsMenuLayerActive()` 로 메뉴·모달이 열린 동안 스스로 물러난다. 네임플레이트의 `TickComponent` 에는 같은 판정이 없어, 인벤토리·메인메뉴·확인 팝업이 열려 게임이 정지된 동안에도 `State.LockedOn`/`State.Engaged` 가 붙은 대상의 네임플레이트가 계속 그려진다(이 태그들은 메뉴를 연다고 걷히지 않는다). 인디케이터 쪽 주석이 밝힌 "메뉴가 덮어 주지 못한다"는 근거가 네임플레이트에도 그대로 적용된다.
- **제안**: 네임플레이트의 가시성 판정 앞에 `IsMenuLayerActive()` 를 한 줄 넣거나, 두 컴포넌트가 공유할 "스크린 스페이스 표시 허용" 질의를 UI 매니저에 하나 두고 양쪽이 부른다.
- **확신도**: 중간 (메뉴 위에도 네임플레이트를 남기는 것이 의도일 수 있다 — 다만 같은 모듈 안에서 두 위젯의 정책이 갈리는 것은 명시된 결정으로 보이지 않는다)

### 6. 🟢 `UWxViewModel_Effect::UpdateEffectState` 는 티커를 멈추면서 핸들을 비우지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:190`, `:199`, `:209`
- **범주**: 버그/정확성
- **문제**: 형제 격인 `UWxViewModel_Ability::UpdateCooldownState` 는 `false` 를 반환하는 자리마다 `TickerHandle.Reset()` 을 하고 그 이유를 주석으로 남겼다(`WxViewModel_Ability.cpp:405-408`, `:423`). 이펙트 쪽은 세 곳 모두 핸들을 남긴 채 `false` 를 반환해, 이후 `TickerHandle.IsValid()` 가 "돌고 있는 티커가 있다"는 뜻이 아니게 되고 `Deinitialize`(`:72-76`)는 이미 제거된 티커에 `RemoveTicker` 를 건다(핸들이 재사용되지 않아 오작동은 아니다). `:209`(`!World`)은 실질 영향도 있다 — 한 번 걸리면 티커가 영구히 멈춰 남은 시간 표시가 언다.
- **제안**: 세 반환 지점에서 `TickerHandle.Reset()` 을 함께 한다. `:209` 는 반환 대신 이번 프레임만 건너뛰고 `true` 를 돌려주는 편이 의미에 맞다.
- **확신도**: 중간

### 7. 🟢 규칙 위반: 티커 델리게이트에 바인딩되는 콜백 5종에 `Handle` prefix 가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:144`, `:146`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:72-73`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h:82`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `FTickerDelegate::CreateUObject` 로 바인딩되는 `UpdateCooldownState`(`WxViewModel_Ability.cpp:38-40`), `FlushActivationRefresh`(`:388-390`), `FlushOwnedTagsRefresh`·`FlushAbilityRebind`(`WxViewModel_AbilitySystem.cpp:220-222`, `:233-235`), `UpdateEffectState`(`WxViewModel_Effect.cpp:64-66`) 5개가 모두 prefix 없이 있다. 같은 모듈의 다른 델리게이트 콜백(`HandleImageLoaded`, `HandleTagChanged`, `HandleResultChosen`, `HandleObservedWidgetActivationChanged` 등)은 규칙을 지키고 있어 티커 계열만 규칙 밖에 있는 셈이다.
- **제안**: `HandleCooldownTick`·`HandleActivationRefreshTick` 처럼 이름을 맞춘다. `UpdateCooldownState` 는 `RefreshBoundAbility`(`WxViewModel_Ability.cpp:217`)에서 직접도 호출하므로, 티커용 얇은 `Handle...` 래퍼를 두고 본체 이름을 유지하는 쪽이 깔끔하다.
- **확신도**: 높음

### 8. 🟢 팝업 서술자 팩토리 4종이 버튼 목록만 다른 복사본이다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:10-79`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:92-99`
- **범주**: 중복/복잡도
- **문제**: `CreateConfirmationOk`/`OkCancel`/`YesNo`/`YesNoCancel` 4개가 `NewObject` → Header/Body 대입 → `FWxConfirmationPopupAction` 을 1~3개 `Add` 하는 동일 골격의 복사본이라 70행을 쓴다. 유일한 호출부(`WxUILibrary.cpp:92-99`)는 `EWxPopupButtonLayout` 을 switch 로 다시 이 4개에 매핑하므로, 같은 정보가 열거형과 함수 이름 양쪽에 이중으로 표현돼 있다. 버튼 조합을 하나 늘리면 두 곳을 함께 고쳐야 한다.
- **제안**: `UWxGamePopupDescriptor::CreateConfirmation(EWxPopupButtonLayout, Header, Body)` 하나로 합치고 조합→결과 배열 매핑을 그 안에 둔다. 4개 정적 함수는 필요하면 얇은 위임으로 남긴다.
- **확신도**: 높음

### 9. 🟢 네임플레이트는 뷰모델 주입 실패를 삼킨다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:110`
- **범주**: 버그/정확성 (진단)
- **문제**: `View->SetViewModelByClass(CharacterViewModel)` 의 반환값을 버린다. 뷰모델 소스가 수동 지정(Creation Type: Manual)이 아니면 주입이 실패하는데, 위젯은 그대로 뜨고 값만 비어 보여 원인을 짚을 단서가 없다. 같은 실패를 `AWxIndicator::BindViewModel` 은 잡아 경고까지 남기고 그 이유를 주석으로 적어 뒀다(`WxIndicator.cpp:97-102`). 같은 함수 안의 다른 실패 경로(ASC 없음 `:82-86`, MVVM View 확장 없음 `:101-106`)는 이미 경고를 남기고 있어 여기만 비어 있다.
- **제안**: `WxIndicator.cpp:97-102` 와 같은 형태로 반환값을 확인해 실패 시 `LogWxUI` 경고를 남긴다.
- **확신도**: 중간 (의도적 생략일 수 있으나, 같은 모듈·같은 함수의 다른 경로와 어긋난다)

### 10. 🟢 자막 태스크는 매 틱 바인딩 프로퍼티를 복사한다 — 형제 노드는 같은 이유로 껐다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp:10-14`, 대비 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:19-21`
- **범주**: 성능/안전
- **문제**: `FWxStateTreeTask_MarkIndicator` 는 생성자에서 `bShouldCopyBoundPropertiesOnTick = false` 를 두고 "완료 없이 매 프레임 도는 태스크라 바인딩 복사가 그대로 프레임 비용이 된다"는 근거를 적어 뒀다. `FWxStateTreeTask_PrintSubtitle` 도 자막이 걸린 동안 계속 `Running` 을 반환하며 매 프레임 `Tick` 하는데(`:38-70`) 이 플래그를 건드리지 않아 엔진 기본값 `true`(`FStateTreeTaskBase` 생성자)가 그대로다. 이 노드가 저작에서 받는 값은 `StartRow` 하나뿐이고 그것은 `EnterState` 와 `ShowRow` 에서만 읽으므로, 틱마다 복사할 이유가 없다.
- **제안**: 생성자에 `bShouldCopyBoundPropertiesOnTick = false` 를 추가하고 MarkIndicator 와 같은 근거 주석을 단다.
- **확신도**: 중간 (프레임당 비용은 작다. 다만 같은 모듈의 형제 노드가 같은 조건에서 반대 선택을 하고 있어 의도된 차이로 보이지 않는다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/` 전 헤더, `Source/WxGame/Character/WxEnemyCharacter.cpp` (뷰모델 소비 측 확인용)
- **미검토 / 한계**: WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이다. `AWxIndicator::UpdateProjection` 의 화면 밖 접힘·역투영 수식은 로직을 따라 읽었으나 실행 검증은 하지 않았고, 성능은 코드 판독 기준이며 프로파일링하지 않았다. 발견 5 는 두 위젯이 실제로 어느 z-order 로 뷰포트에 붙는지까지는 확인하지 않았다. 아래는 이번에 확인해 보고 발견으로 올리지 않은 지점이라, 다음 리뷰가 같은 자리를 다시 파지 않도록 남긴다.
  - 의존 경계는 깨끗하다 — `WxUI.Build.cs` 의 Wx 의존은 `WxCore` 하나뿐이고 `uplugin` 도 같다. 소스가 include 하는 Wx 헤더는 `WxGameplayTags.h`·`WxUIData.h`·`WxLocatorUtils.h` 뿐으로 전부 `WxCore` 소속이다.
  - 소스 56파일 전부의 첫 줄이 `// Copyright Woogle. All Rights Reserved.` 다(규칙 2 충족). `FORCEINLINE` 은 하나도 없고 헤더 인라인 정의는 StateTree 두 노드의 `GetInstanceDataType()` 뿐이며 두 헤더 모두 예외 사유 주석을 달고 있다(규칙 6 충족).
  - `BlueprintCallable` 은 5곳이며 4곳은 BP Function Library(`UWxUILibrary`)와 BP Async Action 팩토리(`UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer`)라 규칙 5 범위 안이다. 남는 `UWxViewModel_Ability::TryActivateAbility`(`WxViewModel_Ability.h:41`)는 앞선 리뷰가 조사해 "VM Command 함수는 승인된 예외"로 걷어낸 항목이라 이번에도 올리지 않았다 — 재논의가 필요하면 그 판단부터 다시 봐야 한다.
  - 람다는 `UWxUILibrary::MakeNativeResultDelegate`(`WxUILibrary.cpp:23-26`)의 `CreateWeakLambda` 하나뿐이고 동적→네이티브 델리게이트 변환이라 규칙 3 위반으로 올리지 않았다. 다만 이 함수와 `MakeSubtitleContext`(`WxViewModel_Subtitle.cpp:15-21`)는 `.cpp` 익명 namespace 헬퍼를 지양한다는 프로젝트 피드백과 어긋난다 — CLAUDE.md 명시 규칙이 아니라 발견으로 세지 않았다.
  - `UWxActivatableWidget::GetDesiredInputConfig` 가 `Super::` 를 부르지 않는 것은 값 반환 가상 함수의 전면 대체라 정상이다. StateTree 두 노드의 `GetDescription`·`PostEditInstanceDataChangeChainProperty` 도 같다.
  - `UWxViewModel::FindSharedViewModel` 의 `GetObjectsWithOuter(..., EGetObjectsFlags::None, ...)` 는 UE 5.8 시그니처상 "중첩 서브오브젝트 제외"를 뜻해(`UObjectHash.h:128`) 직속 자식만 훑는 의도와 일치한다 — 옛 bool 오버로드와 혼동한 것이 아니다.
  - `UWxAsyncAction_PushWidgetToLayer` 의 동기 완료 재진입(`Activate` `:22-60`, `HandleWidgetClassLoaded` `:77-145`)은 `bFinished` 게이트로 모든 분기가 막혀 있다. `UWxViewModel::RequestImageAsync` 의 슬롯 재요청·맵 재해시 처리, `UWxPlayerLayoutComponent::ClearLayout` 의 취소→콜백 재진입도 주석대로 방어돼 있다.
  - 티커 델리게이트는 `CreateUObject` 라 대상 VM 이 수거되면 `IsBound()` 가 풀려 스스로 제거된다 — 댕글링 위험은 없다. `HandleCanUnpause` 도 같은 이유로 서브시스템이 사라지면 `AGameModeBase::ClearPause` 가 엔트리를 걷어낸다.
  - `WantsGamePause` 가 레이어별 `GetActiveWidget` 하나만 보는 것은 CommonUI 스택이 위에 얹힌 위젯 아래를 비활성화하므로 올바르다 — 아래 위젯의 정지 의사를 무시하는 버그가 아니다.
  - `UWxViewModel_Attribute::HandleAttributeChanged`(`:129`)의 `IsAttributeFull` 판정은 Max 미바인딩 시 `Initialize`(`:27`)와 결과가 갈리지만, 유일한 생성 경로인 `GetOrCreateAttributeViewModel`(`WxViewModel_AbilitySystem.cpp:89`)이 Max 를 Current 로 대체해 항상 유효하게 만들므로 도달하지 않는다.
  - 레이아웃 부재·`LayoutClass` 미지정이 조용히 아무것도 안 하는 경로(`WxPlayerLayoutComponent.cpp:55-60`, `WxUIManagerSubsystem.cpp:341-353`)와 `UWxAsyncAction_PushWidgetToLayer` 의 실패 분기에 로그가 없는 것은 실패가 화면에서 즉시 보이는 계약이라 진단 가드를 별 발견으로 올리지 않았다(발견 1 의 제안에 포함).

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 56파일 — `/module-review`로 갱신*
