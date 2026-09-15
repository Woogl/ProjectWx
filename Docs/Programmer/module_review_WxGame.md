# WxGame — 코드 리뷰

> 조립 모듈로서 도메인 경계, 빙의·사망·리스폰 수명, 뷰모델 정리 경로가 탄탄하다. 이번에 실질 동작 결함은 찾지 못했다. 직전 리뷰 1번(원격 클라 폰의 히트스톱 보정)은 WxCombat `UWxHitStopComponent::SetFrozen`에서 서버 사본을 빼는 조건으로 해소됐다. 남은 것은 override 접근 지정자 규칙 위반 2곳과 뷰모델의 죽은 구독 하나다. 소스 57파일을 모두 읽었고, 캐릭터·AI 컨트롤러·리스폰·게임 흐름·뷰모델은 연결된 플러그인과 엔진 소스까지 따라가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟢 InventoryItem 뷰모델의 두 `Initialize`가 아무 일도 하지 않는 인벤토리 도착 구독을 건다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:29`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:39`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:100`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:115`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:151`
- **범주**: 중복/복잡도
- **문제**:
  - `Initialize(Inventory, Instance)`(`:23`)와 `Initialize(Inventory, ItemDef)`(`:34`)는 첫 줄의 `Deinitialize()`가 `StopObserving()`(`:115` → `:95`)으로 `ObservedController`를 비운다. 이후 이 필드를 다시 세우는 곳이 없다.
  - `HandleInventoryReady`는 `ObservedController.IsValid()`일 때만 연결한다(`:100`). 그래서 두 경로가 거는 `OnAnyInventoryReady` 구독(`:29`, `:39`)은 신호를 받아도 항상 무동작이다. 같은 자리의 `OnAnyInventoryEnded` 구독은 인벤토리 종료 시 연결을 끊으므로 유효하다.
  - 이 두 경로는 `UWxViewModel_Inventory`가 슬롯마다(`WxViewModel_Inventory.cpp:151`), 획득마다(`WxViewModel_Inventory.cpp:115`) 부른다. 획득 뷰모델은 교체될 때(`:117`) 해제되지 않으므로, 이 죽은 바인딩도 GC 전까지 정적 델리게이트에 남는다.
  - 코드를 읽는 사람은 이 구독을 보고 "슬롯 뷰모델도 늦게 온 인벤토리에 스스로 붙는다"고 오해하기 쉽다. 실제로 늦은 연결은 `StartObserving` 경로에만 있다.
- **제안**: 두 `Initialize`에서 `ReadyHandle = ...OnAnyInventoryReady.AddUObject(...)` 한 줄씩만 지운다. 핸들이 비어 있어도 `StopObserving`의 `Remove`는 무해하므로 다른 곳은 건드리지 않는다. 헬퍼 추출은 하지 않는다.
- **확신도**: 높음

### 2. 🟢 override 두 곳이 직접 베이스와 다른 접근 지정자로 선언돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:40`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h:47`
- **범주**: 규칙 위반
- **문제**: 사용자 규칙(override는 직접 베이스의 접근 지정자를 그대로 따른다)과 어긋난다. WxUI 리뷰에서 같은 유형(`WxIndicator::BeginPlay`)을 지적해 이미 고친 선례가 있다.
  - `AWxCharacterBase::CanJumpInternal_Implementation`은 public 절에 있다. 직접 베이스 `ACharacter`는 이 함수를 protected로 선언한다(엔진 `Character.h:859`–`:873`).
  - `UWxMetaHumanComponent::OnRegister`·`OnUnregister`는 protected 절에 있다. 직접 베이스 `UMetaHumanComponentUE`는 둘을 public으로 다시 선언한다(엔진 `MetaHumanComponentUE.h:17`–`:20`). 헤더 주석도 `UActorComponent`(protected) 기준으로 적혀 있어 기준 베이스를 잘못 잡았다.
- **제안**:
  - `CanJumpInternal_Implementation` 선언은 protected 절로 옮긴다. cpp 정의도 헤더의 새 순서에 맞춰 옮긴다.
  - `OnRegister`·`OnUnregister` 선언은 public 절의 `SetLeaderMesh` 뒤로 옮기고, Begin/End 주석을 `UMetaHumanComponentUE`로 고친다. cpp 정의는 이미 그 순서라 옮기지 않아도 된다.
  - 호출부가 없고 엔진만 부르는 함수라 동작은 바뀌지 않는다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**:
  - 모듈 소스: `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.h`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Controller/WxAIController.h`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.h`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.h`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`
  - 발견 검증용 외부 코드:
    - 플러그인: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`
    - 엔진(UE 5.8): `Character.h`·`Pawn.h`·`CharacterMovementComponent.h`·`ActorComponent.h`·`MetaHumanComponentUE.h`·`GenericTeamAgentInterface.h`(override 접근 지정자 대조), `GameplayAbility.cpp`·`GameplayEffectTypes.cpp`(활성 태그 복제·태그 맵 역직렬화), `DataChannel.cpp`(서브오브젝트 기록 순서)
    - 워크로그: `.claude/worklog/2026-09-15-AI-컨트롤러-피아-판정-위임.md`, `.claude/worklog/2026-09-12-처형-프롬프트-FText-저작.md`, `.claude/worklog/2026-09-15-인디케이터-BeginPlay-접근지정자.md`, `.codex/worklog/2026-09-12-ViewModel-개선-범위-축소.md`
- **훑은 파일**:
  - 소스: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.h`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Controller/WxPlayerController.h`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Cheat/WxCheatManager.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.h`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.h`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.h`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.h`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`
  - 일괄 검사: 소스 첫 줄 Copyright는 57파일 모두 통과했다. `FORCEINLINE`·`inline`·헤더 본문 정의·람다는 0건이다. override 선언은 전부 직접 베이스의 접근 지정자와 대조했고, 어긋난 것은 발견 2의 두 곳뿐이다.
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE·네트워크 실행은 하지 않았다. BP·WBP 내부와 BP 디폴트 값(플레이어 폰별 AbilitySet, `Team` 설정 등)은 범위 밖이다.
  - 직전 리뷰(`e0106372a`) 항목의 처리:
    - 1번(원격 클라 폰 히트스톱 보정)은 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp:66`에서 서버 사본(`bOnlyAllowAutonomousTickPose`)을 배율에서 빼 해소됐다.
    - 2번은 세 진입점 중복 지적을 빼고 죽은 구독만 남겼다(발견 1). 중복 제거보다 최소 인플레이스 변경을 선호한다는 사용자 결정에 맞췄다.
    - 3번(청각이 Neutral 팀을 적대로 들음)은 코드에 그대로 있다. 다만 09-15 워크로그에서 사용자가 솔버 등록 대신 컨트롤러 위임을 택하고 후속 과제로 기록했으므로 다시 올리지 않았다.
    - 4번(처형 프롬프트는 어빌리티 보유를 보는데 `CanInteract`는 보지 않음)도 코드에 그대로 있다. 모든 플레이어 폰이 처형 어빌리티를 가지면 증상이 없고 영향이 작아 뺐다.
    - 5번(InteractionList의 스캐너 도착 대기 경로는 실행되지 않음)도 코드에 그대로 있다. 동작 영향이 없고, 09-12 범위 축소에서 대기 경로와 그 해제를 유지하기로 한 결정과 겹쳐 뺐다.
  - `UWxAbilitySystemComponent::GiveAbilitySets`는 멱등이라 재빙의 시 재부여 문제는 없다. `AWxEnemyCharacter::BeginPlay`의 `SetReplicationMode(Full)`는 기본값과 같은 무동작 호출이지만 영향이 없어 뺐다.

---
*문서 기준 커밋 `4096004a4` · 리뷰일 2026-09-16 · 소스 57파일 — `/module-review`로 갱신*
