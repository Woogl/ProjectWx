# WxGame — 코드 리뷰

> 조립 모듈로서 도메인 경계, 빙의·사망·리스폰 수명, 뷰모델 정리 경로가 탄탄하다. 직전 리뷰의 override 접근 지정자 지적은 `b2a5ba05`에서 해소됐고, 새로 찾은 실질 문제는 스포너에 부착된 적이 원격 클라에서 이동 복제를 잃는 것(멀티 한정) 하나다. 소스 57파일을 모두 읽었고, 바뀐 `WxCharacterBase`·`WxMetaHumanComponent`와 적 이동 복제는 연결된 플러그인과 엔진 소스까지 따라가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 스포너에 부착된 적은 원격 클라에서 이동 시뮬레이션·속도·이동 모드 복제를 함께 잃는다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:94`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp:78`
- **범주**: 설계/구조
- **문제**:
  - `OnSpawnedBy`는 `FinishSpawning` 전에(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:147`) 캡슐 루트를 스포너에 붙인다. 부착된 액터에 대해 서버는 `ReplicatedMovement`를 채우지 않고 `AttachmentReplication`의 상대 오프셋만 보낸다(엔진 `ActorReplication.cpp:508`). 클라도 부착 중에는 복제된 위치·속도 적용을 건너뛴다(`ActorReplication.cpp:253`–`:269`).
  - 그래서 클라의 `ReplicatedMovement`는 끝까지 0으로 남고, `UCharacterMovementComponent::SimulateMovement`는 이 경우 곧바로 반환한다(`CharacterMovementComponent.cpp:2201`). 네트워크 스무딩만 빠지는 것이 아니다. 시뮬 프록시의 `Velocity` 갱신과, 같은 함수 안에서만 도는 복제 이동 모드 적용(`ApplyNetworkMovementMode`, `:2244`)도 함께 멈춘다.
  - 실패 시나리오: 멀티 클라에서 스포너 적은 넷 업데이트마다 위치가 스냅되고, 속도를 읽는 로코모션은 정지 자세로 미끄러진다. 낙하·착지 때 `UWxCharacterMovementComponent::OnMovementModeChanged`가 불리지 않아 `Movement.InAir` 태그와 착지 섹션 점프(`:78`–`:93`)도 클라에서 빠진다. 배치 적을 경로 액터에 부착하는 규칙(08-29 워크로그)도 같은 경로를 탄다.
  - `WxSpawner.cpp:153`–`:155` 주석과 `.claude/worklog/2026-08-29-정찰-경로-조회-부착-부모로-고정.md`는 "원격 스무딩 이탈"만 감수한 대가로 적었다. 속도·이동 모드까지 사라진다는 점은 기록에 없다. 스탠드얼론과 리슨 서버 호스트 화면에는 영향이 없다.
- **제안**:
  - 부착을 걷고, 경로를 든 액터를 기존 `UWxAIBehaviorComponent`의 약참조로 넘긴다. 스폰 적은 `OnSpawnedBy`가 채우고, 배치 적은 인스턴스 편집값으로 채운다.
  - `UWxPatrolComponent::FindPatrolComponent`(`Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp:25`)는 그 참조를 읽게 한다. `WxSpawner.cpp:171`의 부착 목록 안전망은 빠지고, 주 경로인 약참조 추적만 남는다.
  - 부착을 유지하기로 하면, 최소한 대가 주석과 멀티 검증 항목에 속도·이동 모드 누락을 추가한다.
- **확신도**: 중간 (엔진 경로는 정적으로 확인했지만 PIE 멀티로 재현하지 않았다. 부착 자체는 사용자가 대가를 알고 고른 결정이다)

### 2. 🟢 InventoryItem 뷰모델의 두 `Initialize`가 아무 일도 하지 않는 인벤토리 도착 구독을 건다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:29`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:39`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:100`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:115`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:151`
- **범주**: 중복/복잡도
- **문제**:
  - `Initialize(Inventory, Instance)`(`:23`)와 `Initialize(Inventory, ItemDef)`(`:34`)는 첫 줄의 `Deinitialize()`에서 `StopObserving()`(`:115` → `:95`)을 거쳐 `ObservedController`를 비운다. 이후 이 필드를 다시 세우는 곳은 없다.
  - `HandleInventoryReady`는 `ObservedController.IsValid()`일 때만 연결한다(`:100`). 그래서 두 경로의 `OnAnyInventoryReady` 구독(`:29`, `:39`)은 신호를 받아도 항상 무동작이다. 같은 자리의 `OnAnyInventoryEnded` 구독은 인벤토리 종료 시 연결을 끊으므로 유효하다.
  - 두 경로는 `UWxViewModel_Inventory`가 슬롯마다(`WxViewModel_Inventory.cpp:151`), 획득마다(`WxViewModel_Inventory.cpp:115`) 부른다. 획득 뷰모델은 교체될 때(`WxViewModel_Inventory.cpp:117`) 해제되지 않으므로, 이 죽은 바인딩도 수거 전까지 정적 델리게이트에 남는다.
  - 읽는 사람은 이 구독을 보고 "슬롯 뷰모델도 늦게 온 인벤토리에 스스로 붙는다"고 오해하기 쉽다. 실제로 늦은 연결은 `StartObserving` 경로에만 있다.
- **제안**: 두 `Initialize`에서 `ReadyHandle = ...OnAnyInventoryReady.AddUObject(...)` 한 줄씩만 지운다. 핸들이 비어 있어도 `StopObserving`의 `Remove`는 무해하므로 다른 곳은 건드리지 않는다.
- **확신도**: 높음

### 3. 🟢 `FaceAnimClass` 툴팁이 필드 용도를 잃어 "지정하지 않아도 된다"로 읽힌다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.h:61`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:62`
- **범주**: 버그/정확성
- **문제**:
  - `047197ac` 주석 정리에서 "페이스에 걸 AnimBP (예: ABP_Face)" 문장이 빠지고 "페이스 포스트프로세스 ABP는 메시 에셋에 내장돼 있어 별도 지정이 필요 없다"만 남았다. 이 주석은 `FaceAnimClass`의 에디터 툴팁이 되므로, 기획자는 필드 자체를 비워도 된다고 읽기 쉽다.
  - 페이스는 리더 포즈를 받지 않는다(`.cpp:61`은 부착만 한다). 엔진 `UMetaHumanComponentUE::BeginPlay`도 페이스에는 변수 연결만 하고 팔로우를 걸지 않는다. 따라서 페이스가 몸을 따라가는 경로는 이 AnimClass뿐인데, 비워 두면 `SetAnimInstanceClass(nullptr)`(`.cpp:62`)가 경고 없이 통과해 페이스가 리더 루트에 참조 포즈로 남는다.
- **제안**: 툴팁 앞머리에 필드 용도(바디 포즈를 받는 페이스 메인 AnimBP, 예: ABP_Face)를 되살린다. 코드는 바꾸지 않는다.
- **확신도**: 중간 (페이스 포스트프로세스 ABP가 포즈 복사까지 맡는 에셋 구성이라면 해당 없다)

## 검토 범위
- **깊게 본 파일**:
  - 모듈 소스: `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxPlayerCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.h`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.h`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.h`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`
  - 발견 검증용 외부 코드:
    - 플러그인: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`
    - 엔진(UE 5.8.1): `MetaHumanComponentUE.h/.cpp`·`MetaHumanComponentBase.h/.cpp`(오버라이드 대상·이름 조회·페이스 처리), `Character.h`·`Character.cpp`·`Pawn.cpp`·`Actor.cpp`·`ActorReplication.cpp`·`PackageMapClient.cpp`·`CharacterMovementComponent.cpp`(부착 액터의 이동 복제·시뮬 프록시 경로), `SkinnedMeshComponent.cpp`(숨긴 리더의 틱 조건), `AIController.cpp`(빙의 흐름)
    - 워크로그: `.claude/worklog/2026-09-16-override-접근지정자-정정.md`, `.claude/worklog/2026-08-29-정찰-경로-조회-부착-부모로-고정.md`, `.claude/worklog/2026-07-26-WxWorld-코드리뷰-수정.md`, `.codex/worklog/2026-09-12-ViewModel-개선-범위-축소.md`
- **훑은 파일**:
  - 소스: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.h`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Controller/WxPlayerController.h`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Cheat/WxCheatManager.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.h`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.h`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.h`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.h`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`
  - 일괄 검사: 소스 첫 줄 Copyright는 57파일 모두 통과했다. `FORCEINLINE`·`inline`·헤더 본문 정의는 0건이고, 클래스·구조체·열거형은 모두 `Wx` 접두사를 갖는다. `WxCharacterMovementComponent.cpp:12`의 익명 namespace 상수는 CLAUDE.md 규칙이 아니라 올리지 않았다.
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE·네트워크 실행은 하지 않았다. BP·ABP·WBP 내부와 BP 디폴트 값은 범위 밖이다 — 발견 1의 로코모션 영향은 ABP가 속도를 읽는지에, 발견 3은 페이스 에셋의 ABP 구성에 달려 있다.
  - 직전 리뷰(`4096004a4`) 항목의 처리:
    - 1번(죽은 도착 구독)은 코드와 라인이 그대로라 발견 2로 남겼다.
    - 2번(override 접근 지정자)은 `b2a5ba05`에서 해소됐다(`Source/WxGame/Character/WxCharacterBase.h:70`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h:47`–`:50`). 이 기준은 현재 CLAUDE.md 규칙에 없어 이번에 다시 대조하지 않았다.
    - 그 전 리뷰에서 뺀 항목(청각이 Neutral 팀을 적대로 들음, 처형 프롬프트와 `CanInteract`의 판정 차이, InteractionList의 스캐너 대기 경로)은 코드에 그대로 있으나 같은 사유로 다시 올리지 않았다.
  - 적 처치 보상이 항상 0번 플레이어에게 가는 것(`Source/WxGame/Character/WxEnemyCharacter.cpp:175`)은 코드 주석이 정책으로 명시해 올리지 않았다.
  - 모듈 밖 관찰(WxAI 리뷰 몫): `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`는 매 틱 `MaxWalkSpeed`를 직접 쓰고(`:134`) 해제 때 진입 시점 값으로 되돌린다(`:64`, `:116`). 그 사이 SPD가 바뀌면 `AWxCharacterBase::HandleSPDAttributeChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:208`)가 적용한 값을 덮는다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 57파일 — `/module-review`로 갱신*
