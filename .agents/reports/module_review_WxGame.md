# WxGame — 코드 리뷰

> 조립 모듈로서 상태가 좋다. 빙의·사망·래그돌·리스폰의 수명 경계, 서버 권위 재검증, 뷰모델 구독 해제 경로가 대체로 짜여 있고 위험한 지점마다 근거 주석이 붙어 있다. 이번 리뷰는 소스 57파일을 모두 읽고 캐릭터·컨트롤러·프레임워크·MVVM의 cpp 로직까지 내려갔으며, 판단이 갈리는 지점(ActivationOwnedTags 복제, `GiveAbilitySets` 재진입, ASC 기본 복제 모드)은 연결 플러그인과 UE 5.8 엔진 소스로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 교체된 획득 뷰모델이 해제되지 않아 인벤토리 구독과 아이콘 스트리밍을 물고 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:114`
- **범주**: 버그/정확성
- **문제**:
  - `HandleStackChanged`는 획득(`Delta > 0`)마다 새 `UWxViewModel_InventoryItem`을 만들어 `Initialize(Inventory, ItemDef)`로 인벤토리 델리게이트 4개(`WxViewModel_InventoryItem.cpp:56`–`:59`)에 구독시킨 뒤 `LastAcquiredItem`을 덮는다(`:117`). 교체되는 이전 인스턴스에 `Deinitialize()`를 부르지 않는다.
  - 같은 파일의 다른 두 교체 지점은 모두 해제한다 — `RefreshAllItems`는 목록에서 빠진 자식 VM을 해제하고(`:158`–`:164`), `UnbindSource`는 `LastAcquiredItem`까지 해제한다(`:60`–`:63`). 이 지점만 빠져 있다.
  - 결과: 버려진 VM은 GC 수거 전까지 인벤토리의 멀티캐스트 목록에 남아 스택·슬롯·충전·내용 변경마다 `RefreshFromSource()`를 헛돌리고, `ApplyStaticDataFromDef` → `SetIcon`이 건 비동기 아이콘 스트리밍 핸들(`UWxViewModel::Deinitialize`가 취소하는 그것)도 취소되지 않은 채 남는다. 아이템을 연속으로 줍는 구간에서 누적된다.
- **제안**: `UE_MVVM_SET_PROPERTY_VALUE` 직전에 기존 `LastAcquiredItem`이 있으면 `Deinitialize()`를 부른다. `UnbindSource`가 이미 쓰는 것과 같은 한 줄이다.
- **확신도**: 높음

### 2. 🟢 두 `Initialize` 경로가 절대 발화하지 않는 인벤토리 도착 구독을 건다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:29`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:39`
- **범주**: 중복/복잡도
- **문제**:
  - 두 `Initialize` 오버로드는 첫 줄 `Deinitialize()` → `StopObserving()`(`:113` → `:89`)에서 `ObservedController`를 비우고 다시 채우지 않는다. `HandleInventoryReady`는 `ObservedController.IsValid()`가 참일 때만 연결하므로(`:100`) 이 두 구독은 신호를 받아도 항상 무동작이다. 같은 자리의 `OnAnyInventoryEnded` 구독은 `CachedInventory` 비교라 유효하다.
  - 읽는 사람은 "슬롯 뷰모델도 늦게 도착한 인벤토리에 스스로 붙는다"고 오해한다. 실제 늦은 연결은 `StartObserving` 경로뿐이다.
  - 직전 리뷰(`5eb1a754`)에서 같은 라인으로 올렸고 코드가 그대로라 이월한다.
- **제안**: 두 `Initialize`에서 `ReadyHandle = ...OnAnyInventoryReady.AddUObject(...)` 한 줄씩 삭제한다. 빈 핸들에 대한 `StopObserving`의 `Remove`는 무해하므로 다른 곳은 손대지 않는다.
- **확신도**: 높음

### 3. 🟢 `AWxEnemyCharacter::BeginPlay`의 `SetReplicationMode(Full)`은 아무 일도 하지 않는다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:47`
- **범주**: 중복/복잡도
- **문제**: `UAbilitySystemComponent` 생성자가 이미 `ReplicationMode = Full`로 둔다(UE 5.8 `AbilitySystemComponent.cpp:78`). 형제 클래스는 생성자에서 Mixed를 지정하는데(`Source/WxGame/Character/WxPlayerCharacter.cpp:56`), 여기만 BeginPlay에서 기본값을 다시 쓴다. 읽는 사람은 "적은 BeginPlay 시점에 모드를 바꿔야 하는 이유가 있다"고 읽게 된다.
- **제안**: 두 줄을 지우거나, 의도를 남기려면 `AWxPlayerCharacter`와 같은 자리(생성자)로 옮긴다.
- **확신도**: 높음

### 4. 🟢 체크포인트 서브시스템을 검사 없이 역참조한다
- **위치**: `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp:52`
- **범주**: 성능/안전
- **문제**: `GetGameInstance()->GetSubsystem<UWxCheckpointSubsystem>()->ResetCheckpoint()`가 널 검사 없이 체인을 탄다. 현재 `UWxCheckpointSubsystem`은 `ShouldCreateSubsystem`을 재정의하지 않아 항상 생성되므로 지금은 터지지 않지만, 같은 모듈의 `Source/WxGame/Framework/WxRespawnLibrary.cpp:41`은 같은 서브시스템을 널 검사하고 쓴다. 같은 함수의 다른 실패 경로가 모두 `StatusText`를 남기고 `false`를 돌려주는 것과도 어긋난다.
- **제안**: `if (UWxCheckpointSubsystem* Checkpoint = ...)` 한 겹을 씌운다.
- **확신도**: 중간

### 5. 🟢 `FaceAnimClass` 툴팁이 필드 용도를 잃어 "비워도 된다"로 읽힌다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.h:61`
- **범주**: 버그/정확성
- **문제**: 이 주석은 에디터 툴팁이 되는데 현재 문구("페이스 포스트프로세스 ABP는 메시 에셋에 내장돼 있어 별도 지정이 필요 없다")만 보이면 필드 자체를 비워도 된다고 읽힌다. 페이스 컴포넌트는 리더 포즈를 받지 않고 부착만 되므로(`.cpp:61`), 몸을 따라가는 경로는 `SetAnimInstanceClass(FaceAnimClass)`(`.cpp:62`)뿐이다. 비우면 경고 없이 통과해 페이스가 참조 포즈로 굳는다. 직전 리뷰에서 올렸고 문구가 그대로라 이월한다.
- **제안**: 툴팁 앞머리에 필드 용도(바디 포즈를 받는 페이스 메인 AnimBP)를 되살린다. 코드는 바꾸지 않는다.
- **확신도**: 중간 (페이스 에셋의 포스트프로세스 ABP가 포즈 복사까지 맡는 구성이라면 해당 없다)

### 6. 🟢 헤더가 선언한 "초기화 경계 유효성 확인"이 한 컴포넌트에만 적용돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:28`, `Source/WxGame/Character/WxCharacterBase.cpp:58`
- **범주**: 설계/구조
- **문제**: 헤더는 "직렬화된 BP·레벨 인스턴스를 다루는 초기화 경계에서는 유효성을 확인한다"를 클래스 계약으로 적어 두었다. 그런데 `PostInitializeComponents`는 `AbilitySystemComponent`를 무검사로 두 번 역참조한 뒤(`:58`, `:63`) `WeaponActor`만 검사한다(`:66`). 모듈 전체에서 생성자 서브오브젝트를 검사하는 곳은 이 한 줄뿐이라, 계약과 실제가 어긋난 채 남는다.
- **제안**: 둘 중 하나로 맞춘다 — 계약을 지킬 것이면 ASC 역참조에도 같은 가드를 두고, 지키지 않을 것이면 헤더 문장을 `WeaponActor` 한정으로 좁힌다.
- **확신도**: 중간 (어느 쪽으로 맞출지는 판단 영역이다)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp` 및 위 파일들의 짝 헤더
- **검증용 외부 코드**: `Plugins/WxCombat/.../WxAbilitySystemComponent.cpp`(`GiveAbilitySets` 재진입 가드·`GetAbilityInputActions` 출처), `Plugins/WxCombat/.../WxCombatAttributeSet.cpp`(SPD 기본값 1.0), `Plugins/WxCombat/.../WxAbility_Death.cpp`(사망·래그돌 태그 발행), `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp`(스캔 주기·프롬프트 변경 판정), `Plugins/WxWorld/.../WxSpawner.cpp`, `Plugins/WxInventory/.../WxItemUseComponent.cpp`, `Plugins/WxDialogue/.../WxDialogueActor.cpp`, `Plugins/WxUI/.../WxViewModel.h`, 엔진(UE 5.8) `GameplayAbility.cpp`·`AbilitySystemComponent.cpp`·`GameplayEffectTypes.h`·`GameplayAbilitiesDeveloperSettings.h`
- **일괄 규칙 검사**: 첫 줄 Copyright 57/57 통과. `FORCEINLINE`·`inline`·헤더 내 함수 본문 정의 0건. 타입 `Wx` 접두사 전부 준수. 델리게이트 콜백은 모두 `Handle` 접두사를 달고 있고, `Super::` 호출이 필요한 override에서 누락은 없다. 규칙 위반 범주의 발견은 없다.
- **미검토 / 한계**:
  - 정적 리뷰다. 빌드·PIE·네트워크 실행은 하지 않았고 BP·ABP·WBP 내부와 BP 디폴트 값은 범위 밖이다(발견 5는 페이스 에셋 구성에 달려 있다).
  - 직전 리뷰(`5eb1a754`) 항목 처리: 1번(스포너 부착으로 적의 이동 복제가 `AttachmentReplication`으로만 가던 문제)은 해소됐다 — `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:154`가 부착을 걷고 그 대가를 주석으로 남겼다. 2번·3번은 코드가 그대로라 발견 2·5로 이월했다.
  - 이전 리뷰들이 근거를 달아 제외한 항목은 같은 사유로 이번에도 올리지 않았다 — 처치 보상이 항상 0번 플레이어에게 가는 것(`Source/WxGame/Character/WxEnemyCharacter.cpp:171`, 주석이 정책으로 명시), 청각이 Neutral 팀을 적대로 듣는 것, 처형 프롬프트와 `CanInteract`의 판정 차이, `UWxViewModel_InteractionList`의 스캐너 대기 경로(`StartObserving`이 세운 `ObservedController`를 `Initialize`가 즉시 비워 재연결이 죽는 것). 넷 중 앞 두 개와 마지막은 README가 명시한 최대 4인 권한 모델과는 어긋나므로, 멀티 검증을 시작할 때 다시 꺼내야 한다.
  - `AWxNpc::CanInteract`가 서버 전용 등록부(`FWxStateTreeTask_WaitForInteraction::IsAwaited`)를 클라 스캐너 경로에서 읽는 것(`Source/WxGame/Character/WxNpc.cpp:43`)은 "서버가 곧 클라"라는 프로젝트 전반의 전제를 주석으로 선언하고 있어 올리지 않았다. 원격 클라에서는 NPC가 상호작용 후보로 뜨지 않는다는 뜻이므로, 위의 멀티 검증 묶음에 함께 둔다.
  - `AWxEnemyCharacter::GetInteractionPrompt()`가 스캔 주기마다 플레이어 어빌리티 스펙 전체를 선형 탐색하는 것(`Source/WxGame/Character/WxEnemyCharacter.cpp:139`)은 실제 비용이 무시할 수준이라 발견으로 올리지 않았다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 57파일 — `/module-review`로 갱신*
