# WxGame — 게임 조립 모듈 (기본 게임 모듈)

> 게임의 진입점이자 골격 모듈. GameMode/GameState/Controller 등 프레임워크와 캐릭터 베이스를 직접 소유하고, 전투·인벤토리·AI·UI·월드·사운드·세이브는 각 Wx 플러그인에 위임해 하나의 플레이 가능한 게임으로 조립한다.

## 책임
**담당**
- 게임 프레임워크: `AWxGameMode`(ModularGameplay 프레임워크 컴포넌트 주입, 스폰은 엔진 기본 경로에 위임), `AWxGameState`(컴포넌트 receiver), `AWxPlayerState`(세션 상태 거주처, 현재 비어있음).
- 캐릭터 계층: `AWxCharacterBase`(ASC 직접 소유 + 3개 인터페이스 구현) → `AWxPlayerCharacter`(카메라/입력), `AWxEnemyCharacter`(BehaviorTree/처형 어포던스/보상) → `AWxBossCharacter`.
- 컨트롤러: `AWxPlayerController`(인벤토리 소유·HUD/사망화면 push), `AWxEnemyController`(폰 BT 실행, Perception 위임).
- 입력 배선: `UWxInputConfig`(IMC + Move/Look + 어빌리티 태그 매핑), 게임플레이 어빌리티 `UWxAbility_Interact`·`UWxAbility_UseItem`, AnimNotify(UseItem).
- 위젯-도메인 접착: `MVVM/` 뷰모델·리졸버(Inventory/Item/BossCharacter/PlayerCharacter/InteractionList) — 도메인 데이터를 WBP에 노출.
- 게임 고유 월드 오브젝트: `AWxCheckPoint`(AWxGimmick 상속 모닥불형 부활 지점), `AWxLaserCorridor`(AWxGimmick 상속 트랩).

**경계 (비담당)**
- ASC/AttributeSet/무기/락온/처형 규칙 정의는 [[WxCombat]] (본 모듈은 컴포넌트를 조립·소유만).
- 인벤토리 자료구조·보상 지급(`GrantReward`) 로직은 [[WxInventory]].
- 상호작용 컴포넌트·기믹/StateTree 인프라·Spawner는 [[WxWorld]].
- AI 지각·Blackboard·BT Task·정찰은 [[WxAI]].
- ActivatableWidget/HUDLayout/뷰모델 베이스 등 UI 프레임워크는 [[WxUI]].
- BGM 소스/Chooser 오디오는 [[WxSound]], 저장/영속 복원(`UWxSaveWorldSubsystem`·`UWxPlayerSpawnComponent`)은 [[WxSave]], 팀·어빌리티 베이스 등 공용 정의는 [[WxCore]].

## 의존성
- **주요 의존**: `WxCore` `WxCombat` `WxInventory` `WxUI` `WxWorld` `WxAI` `WxSave`(Public) + `WxSound` `EnhancedInput`(Private). 엔진: `GameplayAbilities`/`GameplayTags`/`GameplayTasks`, `ModularGameplay`(프레임워크 컴포넌트 주입), `ModelViewViewModel`(MVVM), `MotionWarping`, `AIModule`, `CommonUI`, `UMG`.
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 게임 골격 진입점. `InitGame`에서 프레임워크 컴포넌트를 요청 등록 → receiver에 자동 주입. 플레이어 스폰은 엔진 기본 경로에 위임 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | ModularGameplay 컴포넌트 receiver(GameMode가 주입, 무엇이 붙는지 모름) | `Source/WxGame/Framework/WxGameState.h` |
| `AWxCharacterBase` | 플레이어·적 공통 Abstract 베이스. ASC/AttributeSet/장비/모션워핑 직접 소유, 팀·사망·SPD 이동 반영 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라 + Enhanced Input + 어빌리티 입력·락온·상호작용 위젯 소유 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | BT 구동 적. 처형 어포던스(앞잡/뒤잡)·보상 지급, `IWxSpawnableInterface`(`AWxBossCharacter`가 파생) | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 소유 클라이언트 인벤토리(`UWxInventoryManagerComponent`) 소유, HUD/사망화면 push, 캐릭터 사망 바인딩 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxEnemyController` | 폰 BT 실행·BB 컨텍스트 키 세팅, Perception→BB 동기화는 컴포넌트에 위임 | `Source/WxGame/Controller/WxEnemyController.h` |
| `UWxInputConfig` | IMC + Move/Look/Jump/Crouch/Interact + 어빌리티 태그 바인딩 DataAsset | `Source/WxGame/Input/WxInputConfig.h` |

## 확장 포인트 / 규약
- 새 캐릭터/적/보스는 `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`를 BP 상속 후 컴포넌트(무기 `ChildActorClass`, `BehaviorTreeAsset`, `RewardRow`, BGM 태그 등)를 디폴트에서 지정. ASC는 PlayerState가 아닌 캐릭터가 직접 소유(리스폰 시 스탯 재초기화).
- 입력 확장은 `UWxInputConfig` DataAsset의 `AbilityInputBindings`(InputAction→InputTag). 상호작용은 페이로드 운반이 필요해 직접 바인딩(`InteractAction`), 메뉴/UI 입력은 여기 넣지 않고 CommonUI 액션([[WxUI]] `WxHUDLayout`)으로.
- GameMode `FrameworkComponents`(EditDefaultsOnly)에 프레임워크 컴포넌트 클래스를 추가하면 GameState/Controller 등 receiver에 자동 주입(receiver는 무엇이 붙는지 모른다). 새 프레임워크 기능은 컴포넌트로 추가.
- 새 월드 오브젝트/기믹은 [[WxWorld]] `AWxGimmick` 상속(예: `AWxCheckPoint`, `AWxLaserCorridor`). 권위 State만 C++가 확정하고 비주얼은 GimmickStateTree가 담당하는 패턴.
- 재개 지점은 [[WxSave]] `UWxPlayerSpawnComponent`가 저장 좌표를 `StartSpot`으로 주입해 처리(스폰은 엔진 기본 경로). 오토세이브가 `AWxCheckPoint`뿐인 한 사망 부활은 마지막으로 불을 켠 체크포인트가 된다. 신규 세션 시작지점은 레벨의 일반 `APlayerStart`.
- 권한 모델: 인벤토리는 서버 권한 + 소유 연결 전용 복제. 상호작용 선택은 로컬 레지스트리 소유·예측 실행(LocalPredicted)으로 페이로드를 서버 전송. 에너미 처형 어포던스/발동·보상 지급은 서버 권한.
- WBP의 View Bindings에서 Creation Type = Resolver로 `MVVM/`의 리졸버를 선택하면 게임 상태를 [[WxUI]] 뷰모델에 주입(게임 모듈만 양쪽에 의존 가능하므로 이 글루가 유일 통로).

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 게임 부팅(프레임워크 컴포넌트 주입)의 조립 지점. 재개/스탯 복원 위임 경계 파악.
2. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트(ASC/장비/모션워핑)를 조립하는지 = 모듈 경계의 축소판.
3. `Source/WxGame/Controller/WxPlayerController.h` — 인벤토리 소유 위치와 HUD/사망 UI 흐름.
4. `Source/WxGame/WxGame.Build.cs` — 이 모듈이 조립하는 플러그인·엔진 서브시스템 전체 목록.

## 관련
- 함께: [[WxCombat]] · [[WxInventory]] · [[WxWorld]] · [[WxAI]] · [[WxUI]] · [[WxSound]] · [[WxSave]] · [[WxCore]]

---
*문서 기준 커밋 `a9e6ea8` · 생성일 2026-07-19 · 소스 42파일 — `/readme-writer`로 갱신*
