# WxGame — 게임 조립 모듈 (기본 게임 모듈)

> 게임의 진입점이자 골격 모듈. GameMode/GameState/PlayerController 등 프레임워크와 캐릭터 베이스를 직접 소유하고, 전투·인벤토리·AI·UI·월드·사운드·세이브는 각 Wx 플러그인에 위임해 하나의 플레이 가능한 게임으로 조립한다.

## 책임
**담당**
- 게임 프레임워크: `AWxGameMode`(스폰 지점 선택 위임, 저장 트랜스폼 복원, ModularGameplay 프레임워크 컴포넌트 주입), `AWxGameState`(컴포넌트 receiver), `AWxPlayerState`(세션 상태 거주처, 현재 비어있음), `UWxPlayerSpawningComponent`(Lyra 패턴 PlayerStart 선택).
- 캐릭터 계층: `AWxCharacterBase`(ASC 직접 소유 + 인터페이스 구현) → `AWxPlayerCharacter`(카메라/입력), `AWxEnemyCharacter`(BehaviorTree/처형 어포던스/보상), `AWxBossCharacter`.
- 컨트롤러: `AWxPlayerController`(인벤토리 소유·HUD/사망화면 push), `AWxEnemyController`(폰 BT 실행).
- 입력 배선: `UWxInputConfig`(IMC + Move/Look/어빌리티 태그 매핑), 게임플레이 어빌리티 `UWxAbility_Interact`·`UWxAbility_UseItem`, AnimNotify(Footstep/UseItem).
- 위젯-도메인 접착: `MVVM/` 뷰모델·리졸버(Inventory/Item/BossCharacter/PlayerCharacter/InteractionList) — 도메인 데이터를 WBP에 노출.
- 게임 고유 월드 오브젝트: `AWxCheckPoint`(APlayerStart 상속 모닥불), `AWxLaserCorridor`(AWxGimmick 상속 트랩).

**경계 (비담당)**
- ASC/AttributeSet/무기/락온/처형 규칙 정의는 [[WxCombat]] (본 모듈은 컴포넌트를 조립·소유만).
- 인벤토리 자료구조·보상 지급(`GrantReward`) 로직은 [[WxInventory]].
- 상호작용 컴포넌트·레지스트리·기믹/EffectZone/StateTree 인프라는 [[WxWorld]].
- AI 지각·Blackboard·BT Task·정찰·Spawner는 [[WxAI]].
- ActivatableWidget/HUDLayout/뷰모델 베이스 등 UI 프레임워크는 [[WxUI]].
- BGM 소스/Chooser 오디오는 [[WxSound]], 저장/영속 복원은 [[WxSave]], 팀·어빌리티 베이스 등 공용 정의는 [[WxCore]].

## 의존성
- **주요 의존**: `WxCore`, `WxCombat`, `WxInventory`, `WxUI`, `WxWorld`, `WxAI`, `WxSave`(Public), `WxSound`(Private). 엔진: `GameplayAbilities`/`GameplayTags`/`GameplayTasks`, `ModularGameplay`, `ModelViewViewModel`, `EnhancedInput`, `MotionWarping`, `AIModule`, `CommonUI`.
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 게임 골격 진입점. 스폰 선택을 `UWxPlayerSpawningComponent`에 위임, 세이브 트랜스폼 복원 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | ModularGameplay 컴포넌트 receiver(GameMode가 주입) | `Source/WxGame/Framework/WxGameState.h` |
| `AWxCharacterBase` | 플레이어·적 공통 베이스. ASC/AttributeSet/장비/모션워핑 직접 소유 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라 + Enhanced Input + 어빌리티 입력 소유 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | BT 구동 적. 처형 어포던스/보상 지급, `IWxSpawnableInterface` | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 인벤토리(`UWxInventoryManagerComponent`) 소유, HUD/사망화면 push | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxInputConfig` | IMC + Move/Look + 어빌리티 태그 바인딩 DataAsset | `Source/WxGame/Input/WxInputConfig.h` |
| `UWxAbility_Interact` | 주변 스캔 + 입력 트리거 상호작용 실행(레지스트리 연동) | `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h` |

## 확장 포인트 / 규약
- 새 캐릭터/적/보스는 `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`를 BP 상속 후 컴포넌트(무기 `ChildActorClass`, `BehaviorTreeAsset`, `RewardRow`, BGM 태그 등)를 디폴트에서 지정.
- 입력 확장은 `UWxInputConfig` DataAsset의 `AbilityInputBindings`(InputAction→InputTag). 메뉴/UI 입력은 여기 넣지 않고 CommonUI 액션([[WxUI]] `WxHUDLayout`)으로.
- GameMode `FrameworkComponents`(EditDefaultsOnly)에 프레임워크 컴포넌트 클래스를 추가하면 GameState 등 receiver에 자동 주입(GameState는 무엇이 붙는지 모른다).
- 부활/시작 지점은 `AWxCheckPoint`를 배치하고 `PlayerStartTag`를 고유 지정, 레벨당 하나만 `bIsDefaultStart` 지정(Map Check가 검증).

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 게임 부팅·스폰·세이브 복원의 조립 지점, 위임 구조가 한눈에.
2. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트(ASC/장비/모션워핑)를 조립하는지 = 모듈 경계의 축소판.
3. `Source/WxGame/Controller/WxPlayerController.h` — 인벤토리 소유 위치와 HUD/사망 UI 흐름.

## 관련
- 함께: [[WxCombat]], [[WxInventory]], [[WxWorld]], [[WxAI]], [[WxUI]], [[WxSound]], [[WxSave]], [[WxCore]]

---
*문서 기준 커밋 `9554c3c` · 생성일 2026-07-08 · 소스 48파일 — `/readme-writer`로 갱신*
