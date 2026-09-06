# WxGame — 코드 리뷰

> 도메인 플러그인을 엮는 조립 모듈로서 경계가 잘 지켜져 있고, Experience/GameFeature 부트스트랩과 캐릭터·MVVM 수명주기는 함정 주석까지 붙어 있을 만큼 의도가 명확하다. 남은 과제는 범용 로더에 섞여 있는 콘텐츠 정책과, 실패·해제 경로에서 절반만 채워진 계약들이다. 이번 리뷰는 `Source/WxGame` 전 영역을 훑고 Framework·FrontEnd·Character·MVVM·Inventory 의 핵심 cpp 를 깊게 봤으며, 필요한 경우 WxCombat·WxUI·WxInventory 쪽 계약을 교차 확인했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

항상 재현되는 치명적 런타임 오류는 발견하지 못했다. 아래 1~2 는 실패·해제 입력에서 드러나는 정확성 문제, 3~4 는 범용 코드에 섞인 콘텐츠 정책, 5~6 은 수명주기·복원 누락이다.

## 결과

### 1. 🟡 Experience 액션 해제가 `OnGameFeatureLoading` 의 짝을 호출하지 않고, 정리 순서도 활성 순서 그대로다

- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:335`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:80`
- **범주**: 버그/정확성
- **문제**: 활성 경로는 `OnGameFeatureRegistering()` → `OnGameFeatureLoading()` → `OnGameFeatureActivating(Context)` 세 단계를 직접 통과시키는데, 해제 경로는 `OnGameFeatureDeactivating(Context)` → `OnGameFeatureUnregistering()` 두 단계뿐이라 `OnGameFeatureLoading()` 의 짝인 `OnGameFeatureUnloading()` 이 빠져 있다. 또 정리를 `CollectActions` 가 돌려준 활성 순서 그대로 수행하므로, 뒤 액션이 앞 액션의 자원에 의존하는 구성에서는 의존 대상이 먼저 사라진다. 액션 배열은 임의의 `UGameFeatureAction` 을 받는 계약이므로(`WxExperienceDefinition.h:52`), 로드 단계에서 자원을 잡는 스톡·서드파티 액션을 붙이는 순간 해제 누락이 드러난다. 현재 붙어 있는 `UWxGameFeatureAction_AddComponents` 만으로는 재현되지 않는다.
- **제안**: 활성에 성공한 액션 목록을 보관하고, 해제 시 그 역순으로 `OnGameFeatureDeactivating` → `OnGameFeatureUnloading` → `OnGameFeatureUnregistering` 을 대칭으로 호출한다. 동기 액션만 지원할 생각이라면 `EndPlay` 의 pauser 로그(`:86`)를 경고가 아니라 데이터 검증 단계의 거부로 올려 지원 범위를 명시한다.
- **확신도**: 중간(엔진 훅 이름·존재는 UE 5.8 헤더로 재확인 필요, 대칭 누락 자체는 파일 내에서 확정)

### 2. 🟡 컴포넌트 주입 실패와 번들 로드 취소가 Experience `Loaded` 로 흡수된다

- **위치**: `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:140`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:147`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:219`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:346`
- **범주**: 버그/정확성
- **문제**: GameFeature 플러그인 이름 미해석·활성 실패는 `Failed` 로 정확히 갈라내면서, 그 다음 단계인 컴포넌트 주입 실패는 로그만 남기고 `continue` 한다. 번들 로드도 `BindCancelDelegate` 를 완료 콜백과 같은 함수에 물려 취소를 성공으로 취급한다. 두 경로 모두 매니저까지 결과가 올라오지 않으므로 `FinishExperienceLoad` 가 `Loaded` 를 발행하고 GameMode 의 스폰 게이트(`WxGameMode.cpp:67`)가 열린다. 즉 인벤토리·스캐너 같은 필수 컨트롤러 컴포넌트가 통째로 빠진 상태로 판이 시작될 수 있고, 증상은 "UI 가 안 뜬다" 처럼 원인에서 먼 곳에 나타난다.
- **제안**: 에디터 검증(`IsDataValid`)에서 주입 대상 타입과 소프트 참조 유효성을 먼저 걸러 대부분을 개발 시점에 잡는다. 그 위에 액션의 실패를 매니저로 올리는 경로를 두고, 필수/선택 액션을 데이터로 구분해 필수 실패는 `Failed` 로 합류시킨다. 취소 콜백은 완료와 분리해 최소한 경고를 남긴다.
- **확신도**: 높음(코드 경로), 중간(현재 에셋 구성에서의 실제 발생 여부)

### 3. 🟡 위젯별로 만드는 뷰모델 셋에 `DestroyInstance` 가 없어 구독이 GC 까지 남는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:83`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:51`, `Source/WxGame/MVVM/WxViewModel_Quest.h:60`
- **범주**: 설계/구조
- **문제**: 같은 모듈의 `UWxViewModelResolver_Inventory`·`_Item`·`_BossCharacter` 는 `DestroyInstance` 에서 `Deinitialize()` 를 불러 뷰 해제 시점에 구독을 끊는다. 반면 위 세 리졸버는 `CreateInstance` 에서 매번 `NewObject` 로 새 VM 을 만들면서 해제 훅이 없어, 위젯이 사라진 뒤에도 VM 이 GC 로 수거될 때까지 소스 구독이 살아 있다. 특히 `UWxViewModel_InteractionList` 는 스캐너의 `OnListChanged` 마다 `RebuildEntries` 로 프롬프트 수만큼 `UWxViewModel_Interaction` 을 새로 할당하므로(`WxViewModel_InteractionList.cpp:119`), 죽은 위젯의 VM 이 상호작용 후보가 바뀔 때마다 계속 일한다. `UWxViewModel::BeginDestroy` 가 결국 정리하므로 누수는 아니지만, 해제 시점이 정의되지 않은 것은 맞다. `_Ability`·`_PlayerCharacter` 는 ASC 를 Outer 로 공유하는 VM 이라 해제 훅이 없는 것이 의도된 설계이며 이 지적에서 제외한다.
- **제안**: 세 리졸버에 `DestroyInstance` 를 추가해 자기가 만든 인스턴스만 `Deinitialize()` 한다. 겸사겸사 `UWxViewModel_InteractionList::Deinitialize()` 가 `StopObserving()` 을 부르지 않아 `OnAnyScannerReady` 구독이 `BeginDestroy` 까지 남는 점도 같이 맞춘다(`WxViewModel_InteractionList.cpp:47`).
- **확신도**: 높음

### 4. 🟡 범용 Experience 매니저가 HUD 클래스 선택 정책을 안고 있다

- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:27`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:344`, `Source/WxGame/Framework/WxExperienceActionSet.h:43`
- **범주**: 설계/구조
- **문제**: ActionSet 데이터 스키마가 `UWxHUDLayout` 을 직접 알고, 매니저가 ActionSets 를 순회해 "첫 번째 비어 있지 않은 것"을 골라 UI 매니저에 발행한다. 액션 배열이라는 확장 지점을 이미 갖고 있는데 HUD 만 특별 취급하는 셈이라, 여러 콘텐츠의 HUD 합성이나 UI 제거가 필요해지면 범용 로더와 데이터 스키마를 함께 고쳐야 한다. 배열 순서가 묵시적 우선순위가 되는 것도 데이터에서 읽히지 않는다. WxGame 이 WxUI 를 참조하는 것 자체는 규칙 위반이 아니다.
- **제안**: HUD 등록/해제를 담당하는 `UGameFeatureAction` 을 하나 만들어 기존 액션 파이프라인에 태우고, 매니저는 액션 실행과 로드 상태만 관리한다. 충돌·우선순위 정책과 등록 핸들은 `UWxUIManagerSubsystem` 이 소유한다.
- **확신도**: 높음

### 5. 🟡 GameMode 가 FrontEnd 흐름 정책을 직접 호출한다

- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:47`, `Source/WxGame/Framework/WxGameMode.cpp:72`, `Source/WxGame/Framework/WxGameMode.cpp:81`
- **범주**: 설계/구조
- **문제**: 폰 클래스 결정에 `UWxGameFlowSubsystem::GetSelectedPawnClass` 를, 스폰 전후에 `ValidateArrival`·`HoldArrivalPawn` 을 직접 호출한다. FrontEnd 를 별도 콘텐츠로 떼어내도 기본 GameMode 에 이 의존이 남는다. 또 `GetDefaultPawnClassForController_Implementation` 이 인자로 받은 `InController` 를 쓰지 않고 GameInstance 의 단일 선택을 조회하므로, 플레이어별 선택으로 넓히려면 이 계약을 바꿔야 한다. 현재의 단일 로컬 플레이 흐름이 잘못됐다는 판정은 아니다.
- **제안**: GameMode 에는 Experience 확정·서버 스폰 게이트·엔진 override 어댑터만 남기고, "이 컨트롤러의 폰 클래스" 를 묻는 좁은 계약을 통해 FrontEnd 선택을 받는다. 도착 후 입력 보류·스트리밍 대기는 컨트롤러 컴포넌트나 Flow 가 스스로 소유한다.
- **확신도**: 높음

### 6. 🟡 MetaHuman 부착물을 걷어내도 리더 메시의 애니메이션 틱 옵션이 복원되지 않는다

- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더의 `VisibilityBasedAnimTickOption` 을 `AlwaysTickPoseAndRefreshBones` 로 올리지만, `OnUnregister` 는 표시(`SetVisibility(true)`)만 되돌리고 틱 옵션은 그대로 둔다. 리더가 살아남는 해제 경로(BP 리컴파일, MetaHuman 컴포넌트만 제거·재등록, 레벨 스트리밍 아웃)에서는 부착물이 없어진 뒤에도 화면 밖 본 리프레시 비용이 계속 든다. 월드가 통째로 파괴되는 경우에는 영향이 없다.
- **제안**: 리더를 건드리기 전에 원래 표시 상태와 틱 옵션을 멤버에 저장하고, 그 변경을 적용한 등록 주기의 `OnUnregister` 에서 둘 다 복원한다.
- **확신도**: 높음

### 7. 🟢 일반 ViewModel 함수 네 개에 금지된 `BlueprintCallable` 이 붙어 있다

- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.h:47`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:43`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:46`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`
- **범주**: 규칙 위반
- **문제**: `RequestUseConsumable`·`RequestInteract`·`RequestCycle`·`RequestAdvance` 는 일반 ViewModel 의 멤버인데 `BlueprintCallable` 로 노출돼 있다. `CLAUDE.md` 코딩 규칙 5 가 허용하는 Blueprint Function Library / Blueprint Async Action 팩토리에 해당하지 않는다. `WxViewModel_Inventory.h:80` 의 `SetCurrentCategory` 는 `BlueprintSetter` 가 요구하는 지정자이므로 이 지적에서 제외한다.
- **제안**: 기존 WBP 호출부를 확인해 기능을 보존하면서 허용된 진입점(Function Library 등)으로 옮긴다. 지정자만 떼어 기존 바인딩을 깨뜨리지 않는다.
- **확신도**: 높음

### 8. 🟢 입력 액션 콜백 다섯 개가 `Handle` 접두사 규칙을 따르지 않는다

- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:113`, `Source/WxGame/Character/WxPlayerCharacter.cpp:117`, `Source/WxGame/Character/WxPlayerCharacter.cpp:126`, `Source/WxGame/Character/WxPlayerCharacter.cpp:131`, `Source/WxGame/Character/WxPlayerCharacter.cpp:132`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. 모듈의 다른 모든 바인딩(`HandleExperienceAssetsLoaded`, `HandleInventoryReady`, `HandleSPDAttributeChanged` …)은 이를 지키는데 `BindAction` 에 물린 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 만 예외다. 엔진 이름에 맞춰야 하는 `Jump`·`StopJumping` 은 제외한다.
- **제안**: 다섯 콜백의 선언·정의·바인딩을 `Handle` 접두사로 통일한다.
- **확신도**: 높음

### 9. 🟢 기준 이동속도 계산이 두 곳에 그대로 복제돼 있다

- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:226`, `Source/WxGame/Character/WxCharacterBase.cpp:239`
- **범주**: 중복/복잡도
- **문제**: `GetDefault<AWxCharacterBase>(GetClass())->GetCharacterMovement()->MaxWalkSpeed` 를 읽어 SPD 를 곱하는 두 줄이 초기 1회 적용(`InitAbilitySystem`)과 변경 콜백(`HandleSPDAttributeChanged`)에 그대로 두 번 있다. "기준값은 CDO 에서만 읽는다" 는 이 계산의 함정이 두 곳에 걸쳐 있어, 한쪽만 고치면 인스턴스 값을 기준으로 삼는 버그가 조용히 들어온다.
- **제안**: `ApplyWalkSpeedFromSPD(float NewSPD)` 같은 private 헬퍼 하나로 모으고 두 지점이 그것만 호출하게 한다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/` 의 나머지 정의·데이터 에셋 헤더, `Source/WxGame/AbilitySystem/Ability/*`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_*.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`.
- **경계 확인**: `Plugins/WxCombat/.../WxAbilitySystemComponent.cpp` 의 `GiveAbilitySet` 멱등성, `Plugins/WxCombat/.../WxCombatAttributeSet.cpp` 의 `InitSPD(1.f)`(덕분에 `InitAbilitySystem` 의 SPD 곱셈이 0 이 되는 경로는 없다 — 후보였으나 기각), `Plugins/WxUI/.../MVVM/WxViewModel.cpp` 의 `BeginDestroy → Deinitialize` 계약과 `RF_BeginDestroyed` 가드, `Plugins/WxWorld/.../WxCheckpointSubsystem.h`(`ShouldCreateSubsystem` 미오버라이드라 `WxGameFlowSubsystem.cpp:102` 의 무검사 역참조는 안전). `Plugins/GameFeatures/` 에 GameFeature 플러그인이 없어 역참조 금지 규칙 위반 여지도 없다. 모듈 전체에서 `FORCEINLINE`·인라인 정의·Copyright 첫 줄 누락은 0건이며, 델리게이트 콜백의 `Handle` 접두사는 입력 바인딩 5건을 빼면 전부 지켜져 있다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·자동화 테스트(`Wx.FrontEnd.*`, `Wx.MVVM.*`)를 실행하지 않았다. 항목 1 의 엔진 훅 이름·존재는 이 환경에 UE 5.8 소스가 없어 헤더로 확인하지 못했고, 파일 내부의 대칭 누락만으로 판정했다. `UWxGameFlowSubsystem` 의 상태 기계는 코드로 전 경로를 따라갔으나 실제 PIE 트래블·타임아웃·복귀를 재현하지 않았다. 리플리케이션 관련 판단(Experience 복제 순서, ASC 어트리뷰트 도착 순서)도 실행 검증이 아니다. BP/WBP 내부 구조와 데이터 에셋 내용은 범위 밖이다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 80파일 — `/module-review`로 갱신*
