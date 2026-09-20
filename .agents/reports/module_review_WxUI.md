# WxUI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 수명주기·재진입·파괴 중 브로드캐스트 같은 UI/MVVM의 전형적 함정이 대부분 의도적으로 방어돼 있고(`RF_BeginDestroyed` 가드, 스트리밍 취소, 맵 재해시 주의), 플러그인 경계·명명·인라인 금지 등 프로젝트 규칙 위반도 사실상 없다. 이번 리뷰는 `Plugins/WxUI` C++ 58파일 전부를 훑고 UIManager 서브시스템·비동기 푸시 액션·MVVM VM 트리·인디케이터 투영·StateTree 태스크는 cpp 본문까지 읽었으며, 의심 지점은 UE 5.8 엔진 원본(`UGameInstance::Shutdown`, `UObject::ConditionalBeginDestroy`, `UWorld::SpawnPlayActor`, `GetObjectsWithOuter`, `UWidgetComponent::TickComponent`)과 대조해 오탐을 걷어냈다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 사망 화면만 푸시 추적·태그 해제 대응이 빠져 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:287-298`
- **범주**: 버그/정확성
- **문제**: 같은 파일의 대화 창 경로(`:300-315`, `:317-333`, `:335-349`)는 ①진행 중 푸시를 `PendingDialogueScreenPush` 로 붙들어 취소하고 ②`NewCount <= 0` 에 창을 닫고 ③관찰을 놓는 `WatchPawnTags(:269-270)` 에서도 닫는다. 사망 경로는 이 셋이 전부 없고, 푸시 결과조차 보관하지 않는다. 회수는 전적으로 `Source/WxGame/Framework/WxRespawnLibrary.cpp:79` 이 위젯 자신을 인자로 받아 `DeactivateWidget()` 하는 데 의존한다 — 즉 **플레이어가 그 화면의 부활 버튼을 눌러야만 닫힌다**. 실패 시나리오 두 가지: (a) 부활 버튼이 아닌 경로로 `Ability.Death` 가 걷히면(리바이브 GE, 폰 교체, 치트) 화면이 Menu 레이어에 영구히 남고 관찰도 이미 끊겨 걷을 신호가 오지 않는다. (b) 비동기 클래스 스트리밍이 끝나기 전에 부활하면 부활 **후에** 사망 화면이 떠오르고, 취소할 핸들이 없어 그대로 남는다.
- **제안**: 대화 창과 같은 모양으로 맞춘다 — `PendingDeathScreenPush`·`DeathScreen` 을 들고, `HandleDeathTagChanged` 의 `NewCount <= 0` 분기와 `WatchPawnTags` 에서 `CloseDeathScreen()` 을 부른다. 부활 라이브러리의 `DeactivateWidget()` 은 그대로 둬도 이중 호출이 무해하다.
- **확신도**: 중간

### 2. 🟡 슬롯 VM 마다 ASC 제네릭 태그 이벤트를 구독해, 태그 한 번 변화가 슬롯 수만큼의 전체 재평가를 부른다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:31`
- **범주**: 성능/안전
- **문제**: 어빌리티 슬롯 VM 은 하나하나가 `RegisterGenericGameplayTagEvent()` 에 붙는다(`:31`). 제네릭 이벤트는 부모 태그까지 통지하고 GE 하나가 태그를 여럿 부여하므로, 전투 중 한 프레임에 수십 회 발화한다. 각 발화는 `HandleTagChanged(:432)` → `ScheduleActivationRefresh(:446)` 로 슬롯별 다음-틱 타이머를 걸고, 다음 프레임의 `FlushActivationRefresh(:515)` 는 `RefreshBoundAbility(:173)`(ActivatableAbilities 전수 순회 + `DoesAbilitySatisfyTagRequirements`) 와 `RefreshActivationState(:536)`(다시 전수 순회 + `CanActivateAbility`, 내부에서 쿨다운 GE 조회·비용 판정까지) 를 돈다. 슬롯 N개면 프레임당 N회, 여기에 `WxViewModel_AbilitySystem.cpp:40` 의 제네릭 구독 1개가 더해진다. 프레임당 합계는 `N × 2 × 부여된 어빌리티 수` 순회이며, 코얼레싱은 슬롯 **내부**에서만 일어나고 슬롯 **간**에는 없다.
- **제안**: 코얼레싱 지점을 허브로 올린다 — `UWxViewModel_AbilitySystem` 이 제네릭 태그 이벤트를 혼자 구독해 프레임당 한 번만 모은 뒤(`FlushAbilityRebind(:283-295)` 이 이미 그 모양이다) 자식 슬롯 VM 에 재평가를 밀어 주고, 슬롯 VM 의 개별 구독은 걷는다.
- **확신도**: 중간

### 3. 🟡 비동기 푸시 부기가 세 곳에 복제돼 있고, 네 번째 호출부가 그것을 빠뜨렸다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp:66-89`
- **범주**: 중복/복잡도
- **문제**: "보류 요청을 UPROPERTY 로 붙든다 → `SetCompletionCallback` → `Activate()` → 완료 콜백에서 null 로 놓는다 → 정리 시 `Cancel()`" 라는 동일한 8~10줄 부기가 `WxPlayerLayoutComponent.cpp:66-89/77-81/83-89`, `WxHUDLayout.cpp:80-96`, `WxUIManagerSubsystem.cpp:300-349` 세 곳에 거의 글자 그대로 복제돼 있다. 발견 1의 사망 화면 경로가 바로 이 패턴을 빠뜨린 네 번째 호출부이며, 복제 패턴은 앞으로도 같은 누락을 반복시킨다.
- **제안**: 이 부기를 `UWxAsyncAction_PushWidgetToLayer` 쪽으로 흡수한다 — 예: 소유자와 완료 델리게이트를 받아 보류 핸들을 스스로 관리하는 네이티브 헬퍼(`PushAndTrack`) 하나를 두고, 호출부는 취소만 부른다.
- **확신도**: 높음

### 4. 🟡 활성 이펙트 하나마다 코어 티커를 하나씩 단다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:66-68`
- **범주**: 성능/안전
- **문제**: 지속시간이 있는 GE 마다 `FTSTicker::GetCoreTicker().AddTicker(...)` 가 하나씩 붙는다. 아이콘을 가진 버프가 M개면 코어 티커 M개가 매 프레임 돌고, 각 티커는 `UpdateEffectState(:197)` 에서 `GetActiveGameplayEffect` 조회와 MVVM 필드 4개 세팅을 한다. 형제 VM 들은 각기 다른 펌프를 쓴다 — `WxViewModel_Ability` 는 월드 타이머, `WxViewModel_AbilitySystem` 은 월드 타이머, `WxViewModel_Effect` 만 코어 티커다. 확립된 방침(공유 주기는 베이스의 횡단 서비스 = 정적 레지스트리 + 티커 1개)과 어긋난다.
- **제안**: `UWxViewModel` 베이스에 정적 레지스트리 + 티커 1개짜리 횡단 서비스를 두고, 주기 갱신이 필요한 VM 은 자기를 등록만 한다. VM 자체의 자기완결성(등록/해제를 스스로 한다)은 그대로 유지된다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 자기완결성을 우선한 선택으로 볼 여지가 있다)

### 5. 🟢 `UWxViewModel_Ability::TryActivateAbility` 의 `BlueprintCallable`
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:42-43`
- **범주**: 규칙 위반
- **문제**: 모듈 내 `BlueprintCallable` 4개 중 셋은 규칙이 허용하는 자리다(`WxUILibrary.h:37,40` 은 BP Function Library, `WxAsyncAction_PushWidgetToLayer.h:29` 는 Async 팩토리). 이것만 ViewModel 위의 일반 명령 함수이며, 허용 예외인 "위젯 서브클래스의 MVVM 1-arg setter" 에도 해당하지 않는다.
- **제안**: 명령 통로가 필요하면 `UWxUILibrary` 파사드에 `TryActivateAbility(UWxViewModel_Ability*)` 로 내리거나, 예외로 유지한다면 그 사유를 헤더 주석에 남긴다(StateTree `GetInstanceDataType()` 예외와 같은 방식).
- **확신도**: 낮음(의도된 설계일 수 있음 — WBP 버튼이 VM 을 직접 부르는 MVVM 커맨드 패턴)

### 6. 🟢 `.cpp` 내 익명 namespace 헬퍼
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`
- **범주**: 규칙 위반
- **문제**: 모듈 전체에서 유일한 익명 namespace 헬퍼(`MakeNativeResultDelegate`)다. 호출부가 `:83` 한 곳뿐이라 인라인으로 내려도 3줄이며, "cpp 내부 헬퍼는 익명 namespace 대신 호출부 인라인" 이라는 프로젝트 선호와 어긋난다.
- **제안**: `ShowConfirmationPopup` 안으로 인라인한다.
- **확신도**: 중간

### 7. 🟢 `FlushActivationRefresh` 가 `RefreshActivationState` 를 두 번 돌린다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:515-523`
- **범주**: 중복/복잡도
- **문제**: `RefreshBoundAbility(:520)` 은 조기 반환(물고 있던 어빌리티가 그대로)하지 않는 **모든** 경로에서 끝에 `RefreshActivationState()` 를 부른다(`:232`, `:271`). 그 뒤 `:522` 가 무조건 한 번 더 부르므로, 어빌리티가 갈린 프레임에는 전수 순회 + `CanActivateAbility` 가 연달아 두 번 돈다. 발견 2의 비용이 그만큼 더 커진다.
- **제안**: `RefreshBoundAbility` 가 "대상이 바뀌었는지" 를 bool 로 돌려주게 하고, `FlushActivationRefresh` 는 바뀌지 않았을 때만 `RefreshActivationState()` 를 부른다.
- **확신도**: 높음

### 8. 🟢 HUD 푸시 트리거가 하나뿐이라 레이아웃이 아직 없으면 조용히 영영 안 뜬다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp:54-58`
- **범주**: 설계/구조
- **문제**: HUD 를 띄우는 유일한 트리거가 `OnPossessedPawnChanged` 인데, 그 시점에 레이아웃이 없으면 로그도 재시도도 없이 반환한다. 권위 경로에서는 안전함을 확인했다 — 엔진 `UWorld::SpawnPlayActor` 가 `SetPlayer()`(→ 레이아웃 생성) 를 `PostLogin()`(→ `RestartPlayer` → 빙의) 보다 먼저 부른다(`LevelActor.cpp:1110-1111`). 다만 클라이언트에서는 `OnRep_Pawn` 과 로컬 플레이어 결합(`ULocalPlayer::ReceivedPlayerController`) 의 선후가 같은 방식으로 보장되지 않아, 순서가 뒤집히면 HUD 가 끝까지 뜨지 않고 단서도 남지 않는다.
- **제안**: 레이아웃이 없을 때 경고 로그 한 줄이라도 남기거나, `UWxUIManagerSubsystem` 이 레이아웃 생성 완료를 알리는 신호를 내고 컴포넌트가 그때 한 번 따라잡게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — README 가 v1 싱글/리슨 호스트 전제를 명시한다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp` 및 대응 헤더 전부
- **확인해 두고 지적하지 않은 것**(오탐 배제 근거):
  - 플러그인 경계 — `.Build.cs` 의존성과 실제 `#include` 를 함께 훑었고, WxCore 외 Wx 플러그인 참조는 0건이다(외부 include 는 `WxGameplayTags.h`·`WxUIData.h`·`WxLocatorUtils.h` 뿐, 모두 WxCore).
  - 규칙 준수 — 58파일 전부 첫 줄 저작권 표기 있음, `FORCEINLINE`·헤더 내 함수 본문 0건(StateTree `GetInstanceDataType()` 두 곳은 예외 사유 주석이 붙어 있다), 델리게이트 콜백 `Handle` prefix 일관, override 의 `Super::` 호출 누락 없음.
  - `RF_BeginDestroyed` 가드 — 파괴 중 브로드캐스트를 막는 VM 들의 가드는 올바르다. `UObject::ConditionalBeginDestroy(Obj.cpp:1314-1316)` 가 `BeginDestroy()` 호출 **전에** 플래그를 세운다.
  - `UWxUIManagerSubsystem::Deinitialize(:51)` 의 "로컬 플레이어가 먼저 사라진다" 는 주석은 사실이다. `UGameInstance::Shutdown(GameInstance.cpp:157-167)` 이 `RemoveLocalPlayer` 루프를 `SubsystemCollection.Deinitialize()` 보다 먼저 돈다.
  - `FindSharedViewModel` 의 "언리처블은 기본으로 빠진다" 도 사실이다(`UObjectHash.cpp:1600-1601` 의 `AddMandatoryInternalExclusionFlags`).
  - `UWxNameplateComponent` 가 생성자에서 `SetVisibility(false)` 하는데도 틱이 죽지 않는다. `UWidgetComponent` 의 틱 자동 차단은 `TickMode != Enabled` 에서만 동작하고 기본값은 `Enabled` 다(`WidgetComponent.cpp:58`, `:1264`).
- **미검토 / 한계**:
  - **WBP/에셋은 범위 밖이다.** WxUI 는 위젯 계층·MVVM 바인딩 그래프·이벤트 그래프가 WBP 에 있는 모듈이라 C++ 만으로는 그림이 불완전하다. 특히 `meta=(BindWidget)` 계약 충족 여부, `UWxPrimaryGameLayout::LayerTags` 의 실제 저작 순서(z-order), 각 위젯의 뷰모델 소스가 `Creation Type: Manual` 인지, `UWxUIDeveloperSettings` 4종 클래스가 `DefaultGame.ini` 에 실제로 물려 있는지는 확인하지 않았다. 위 발견은 모두 C++ 근거로만 적었다.
  - `UWxViewModel_Ability` 의 쿨다운·충전 계산이 실제 쿨다운 GE 저작(스택 정책·Duration 산식)과 맞는지는 WxCombat 의 GE 에셋을 봐야 판정할 수 있어 검증하지 않았다.
  - 네트워크 경로(클라이언트에서의 PC/Pawn 복제 순서)는 엔진 코드를 끝까지 따라가지 않았다 — 발견 8의 확신도를 낮음으로 둔 이유다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 58파일 — `/module-review`로 갱신*
