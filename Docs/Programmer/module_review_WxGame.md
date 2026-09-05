# WxGame — 코드 리뷰

> 게임 조립과 도메인 경계는 대체로 명확하며, 이번 검토에서 심각 결함은 확인하지 못했다. FrontEnd 진입·복구, Experience 수명, 캐릭터 초기화·교전·입력, MVVM 연결을 중심으로 검토했으며, UI 구독 수명과 초기 연결 순서에서 수정할 문제를 확인했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 보스 위젯 하나가 해제되면 같은 리졸버를 쓰는 나머지 위젯의 구독도 끊긴다

- **위치**: `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp:44`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp:26`
- **범주**: 버그/정확성
- **문제**: 리졸버는 위젯 클래스가 공유하지만 `DestroyInstance`는 어느 뷰가 해제됐는지 구분하지 않고 `OnAnyBossEngagementChanged.RemoveAll(this)`를 호출한다. 같은 위젯 클래스의 인스턴스 A와 B를 생성한 뒤 A만 제거하면, 살아 있는 B도 이후 보스 교전 시작·종료·사망 통지를 받지 못한다. `CreateInstance`도 기존 구독을 지우고 하나만 다시 등록하므로 인스턴스별 구독이 남아 있지 않다. UE 5.8의 `MVVMViewClass.cpp`는 생성 시 공유 `Source.Resolver->CreateInstance`, 해제 시 같은 리졸버의 `DestroyInstance`를 호출하므로 엔진에서 이를 대신 참조 계수하지 않는다.
- **제안**: 구독을 월드별 관찰 객체에 옮기거나, 살아 있는 뷰를 추적하여 마지막 뷰가 해제될 때만 구독을 제거한다. 같은 클래스의 위젯 두 개 중 하나를 제거한 뒤 남은 위젯의 교전·사망 갱신을 검증한다.
- **확신도**: 높음

### 2. 🟡 고정 아이템 뷰모델은 인벤토리가 늦게 도착하면 계속 빈 상태로 남는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Item.cpp:189`, `Source/WxGame/MVVM/WxViewModel_Item.cpp:195`
- **범주**: 버그/정확성
- **문제**: `ItemToDisplay`를 지정한 리졸버는 생성 시 한 번만 인벤토리를 찾고, 없으면 초기화하지 않은 뷰모델을 그대로 반환한다. 원격 클라이언트에서 HUD 생성이 인벤토리 컴포넌트 복제보다 앞서면 이름·아이콘·수량이 빈 채로 남으며 `CachedInventory`가 없어 사용 요청도 실패한다. HUD의 빙의 처리에는 인벤토리 준비를 기다리는 조건이 없고, 이 뷰모델에는 재조회나 `OnAnyInventoryReady` 구독이 없다. 같은 모듈의 `UWxViewModel_Inventory::StartObserving`은 이미 이 도착 순서를 지원한다.
- **제안**: 고정 아이템 뷰모델도 해당 PlayerController의 `OnAnyInventoryReady`를 기다렸다가 같은 인스턴스를 초기화하고, 연결·소멸 시 구독을 해제한다. 인벤토리가 없는 상태에서 생성한 뒤 컴포넌트가 도착하는 순서로 검증한다.
- **확신도**: 높음

### 3. 🟡 MetaHuman 부착물을 제거해도 리더 메시의 애니메이션 틱 설정이 복원되지 않는다

- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더의 `VisibilityBasedAnimTickOption`을 `AlwaysTickPoseAndRefreshBones`로 바꾸지만, `OnUnregister`는 표시만 켜고 틱 옵션을 복원하지 않는다. MetaHuman 컴포넌트만 등록 해제하거나 제거하여 리더가 계속 살아 있는 경우, 바디가 없어져도 화면 밖 본 리프레시 비용이 계속 발생한다. 월드 전체가 함께 파괴되는 경우에는 지속 비용이 없으므로 문제 범위는 리더가 생존하는 해제 경로이다.
- **제안**: 리더 설정을 변경하기 전에 원래 틱 옵션과 표시 상태를 저장하고, 해당 변경을 적용한 등록 주기의 해제에서 복원한다. 리더를 유지한 채 MetaHuman만 등록·해제해 원래 옵션으로 돌아오는지 확인한다.
- **확신도**: 높음

### 4. 🟢 입력 액션 콜백 다섯 개가 `Handle` 명명 규칙을 따르지 않는다

- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:113`, `Source/WxGame/Character/WxPlayerCharacter.cpp:117`, `Source/WxGame/Character/WxPlayerCharacter.cpp:126`, `Source/WxGame/Character/WxPlayerCharacter.cpp:131`, `Source/WxGame/Character/WxPlayerCharacter.cpp:132`
- **범주**: 규칙 위반
- **문제**: `AGENTS.md` 코딩 규칙 4는 델리게이트 콜백의 `Handle` 접두사를 요구한다. `BindAction`에 연결된 `Move`, `Look`, `ToggleCrouch`, `AbilityInputTriggered`, `AbilityInputReleased`는 이를 따르지 않는다. 엔진 함수명에 맞춰야 하는 `Jump`·`StopJumping`은 이 지적에서 제외한다.
- **제안**: 프로젝트가 정의한 다섯 콜백의 선언·정의·바인딩을 `Handle` 접두사로 통일한다.
- **확신도**: 높음

### 5. 🟢 일반 ViewModel 함수 네 개에 금지된 `BlueprintCallable`이 붙어 있다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Item.h:47`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:43`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:46`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`
- **범주**: 규칙 위반
- **문제**: `RequestUseConsumable`, `RequestInteract`, `RequestCycle`, `RequestAdvance`는 일반 ViewModel의 함수이지만 `BlueprintCallable`로 노출되어 있다. `AGENTS.md` 코딩 규칙 5의 허용 대상인 Blueprint Function Library 또는 Blueprint Async Action 팩토리에 해당하지 않는다. `SetCurrentCategory`의 BlueprintSetter 연계와는 별개의 네 지점이다.
- **제안**: 기존 BP·MVVM 호출 사용처를 확인하여 기능을 보존하면서 허용된 Function Library 진입점 등으로 옮긴다. 지정자만 제거하여 기존 바인딩을 깨뜨리지 않는다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/FrontEnd/Tests/WxFrontEndTests.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/Component/WxAIBehaviorComponent.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.h`, `Source/WxGame/MVVM/WxViewModel_Item.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h`.
- **경계 확인**: WxUI의 `UWxViewModel_Character`·`UWxViewModel` 공유·해제 구현, `UWxHUDComponent` 빙의 후 HUD 생성 조건, `UWxUIManagerSubsystem` 준비 조건, WxInventory의 `OnAnyInventoryReady` 발행·인벤토리 조회, UE 5.8 `MVVMViewClass.cpp`의 리졸버 생성·해제 계약을 확인했다. 소스 전체에 저작권 첫 줄·`BlueprintCallable`·인라인·람다 패턴 검색을 수행했다.
- **이전 리뷰 재판정**: 투사체·소환 매니저의 베이스 무조건 생성과 `bEngaged` 멤버 이중 상태는 현재 코드에서 제거되어 닫았다. MetaHuman 틱 복원과 입력 콜백 명명 문제는 재확인하여 유지했다. ASC 초기화 가드 부족은 현재 유효한 실패 경로를 확인하지 못해 이번 액션 목록에서 제외했다. 이전 문서의 `BlueprintCallable` 문제 없음 판정은 위 네 지점을 확인하여 정정했다.
- **미검토 / 한계**: 정적 코드 리뷰이며 빌드·자동화 테스트·PIE·패키징 실행은 하지 않았다. 특히 인벤토리 복제 지연과 위젯 동시 인스턴스 시나리오는 코드 계약으로 확인했으며 실행 재현은 하지 않았다. BP/WBP 내부 구조, 실제 Experience·ActionSet·아이템 데이터, MetaHuman 엔진 플러그인의 내부 리그 평가, 모든 헤더의 전면 통독은 범위 밖이다. FrontEnd의 현재 싱글플레이 범위 자체는 결함으로 분류하지 않았다. 소스 코드는 수정하지 않았다.

---
*문서 기준 커밋 `3025580b` · 리뷰일 2026-09-05 · 소스 74파일 — `/module-review`로 갱신*
