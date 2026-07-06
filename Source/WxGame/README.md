# WxGame — 기본 게임 모듈 (부트스트랩·프레임워크·캐릭터)

> 도메인 플러그인들을 조립해 실제 플레이 가능한 게임을 구성하는 최상위 모듈. GameMode/GameState/Controller/Character 등 언리얼 게임 프레임워크 액터와, 각 도메인 시스템을 이어 붙이는 접착(glue) 코드를 소유한다.

## 책임
**담당**
- 게임 프레임워크 액터: `AWxGameMode`, `AWxGameState`, `AWxPlayerController`, `AWxPlayerState`
- 스폰/부활 지점 선택: `UWxPlayerSpawningComponent` (Lyra PlayerSpawning 패턴, ModularGameplay 컴포넌트로 GameState 에 주입)
- 캐릭터 계층: `AWxCharacterBase`(ASC 직접 소유·팀·사망 공통) → `AWxPlayerCharacter` / `AWxEnemyCharacter` → `AWxBossCharacter`
- 에너미 AI 컨트롤러 골격: `AWxEnemyController` (BB 세팅·BT 실행)
- 플레이어 입력 구성: `UWxInputConfig` (Enhanced Input Action ↔ Gameplay Tag 매핑)
- 모듈 특화 어빌리티/노티파이: `WxAbility_Interact`, `WxAbility_UseItem`, `WxAnimNotify_Footstep`, `WxAnimNotify_UseItem`
- 플러그인 간 접착: WxUI 뷰모델에 게임 상태를 주입하는 MVVM 리졸버/뷰모델(`WxViewModelResolver_*`, `WxViewModel_*`)
- 프레임워크에 밀착된 월드 오브젝트: `AWxCheckPoint`(APlayerStart 상속 모닥불), `AWxLaserCorridor`

**경계 (비담당)**
- 전투/ASC 어트리뷰트·이펙트: [[WxCombat]] (`UWxAbilitySystemComponent`, `UWxCombatAttributeSet`, `UWxEquipmentComponent`)
- 인벤토리/보상: [[WxInventory]] (`UWxInventoryManagerComponent`, `WxRewardTableRow`)
- UI 위젯·MVVM 베이스: [[WxUI]] (`UWxActivatableWidget`, `UWxViewModel_Character` — 게임 모듈을 참조 못 하므로 주입은 이 모듈 리졸버가 수행)
- AI 인지/BehaviorTree Task: [[WxAI]] (`UWxAIPerceptionComponent`, `UWxBTTask_Patrol`, `UWxPatrolComponent`)
- 상호작용 컴포넌트·기믹·스포너: [[WxWorld]] (`UWxInteractionComponent`, `AWxSpawner`)
- 세이브/월드 상태 복원: [[WxSave]] (`UWxPersistenceGameSubsystem`/`UWxPersistenceWorldSubsystem`)
- BGM/발소리 사운드 소스: [[WxSound]] (`UWxBGMSourceComponent`)
- 공용 정의(팀 타입·인터페이스 등): [[WxCore]]

## 의존성
- **주요 의존**: [[WxCore]], [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxSave]] (Public), [[WxSound]] (Private)
- 특징적 엔진 서브시스템: `GameplayAbilities`, `ModelViewViewModel`, `ModularGameplay`, `EnhancedInput`, `MotionWarping`, `AIModule`, `CommonUI`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | InitGame 에서 프레임워크 컴포넌트 주입 등록, ChoosePlayerStart 를 스폰 컴포넌트에 위임 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | ModularGameplay 컴포넌트 receiver (스폰/TimeDilation 등 자동 주입) | `Source/WxGame/Framework/WxGameState.h` |
| `UWxPlayerSpawningComponent` | 저장 태그 → "Default" 태그 순으로 부활 지점 선택 | `Source/WxGame/Framework/WxPlayerSpawningComponent.h` |
| `AWxPlayerController` | 인벤토리 소유(소유 클라 복제), HUD/데스스크린 푸시, 캐릭터 사망 바인딩 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스(ASC/팀/장비/사망) | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라·입력·락온·HUD 소유 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | AI 제어 적, 처형 어포던스·처치 보상 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `UWxInputConfig` | IMC + 이동/시선 + 어빌리티 입력 태그 매핑 DataAsset | `Source/WxGame/Input/WxInputConfig.h` |

## 확장 포인트 / 규약
- 프레임워크 컴포넌트 추가: `AWxGameMode::FrameworkComponents`(EditDefaults)에 `UGameFrameworkComponent` 파생을 넣으면 receiver 액터(GameState 등)에 자동 주입된다 — GameState 코드 수정 불필요.
- 새 부활 지점 유형: `APlayerStart`(또는 `AWxCheckPoint`)를 배치하고 `PlayerStartTag` 를 지정하면 스폰 컴포넌트의 태그 탐색에 편입된다.
- 캐릭터 파생: `AWxCharacterBase`(Abstract)를 상속하고 BP 에서 `WeaponActor` ChildActorClass·`UIData`·`Team` 등을 지정. 사망 연출은 `HandleDeath()` override.
- 새 플레이어 입력: `UWxInputConfig` 에셋에 InputAction 을 추가하고, 어빌리티 입력은 `AbilityInputBindings` 로 Gameplay Tag 에 매핑.
- WxUI 위젯에 게임 상태 노출: 게임→UI 단방향 참조 제약 때문에, WBP View Bindings 의 Resolver 로 `WxViewModelResolver_*` 를 선택해 이 모듈이 주입을 대행한다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 부트스트랩·컴포넌트 주입·스폰 위임의 진입점. 모듈이 프레임워크를 어떻게 조립하는지 전체 그림.
2. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터 계층의 뿌리. ASC 소유·팀·장비·사망 흐름이 여기서 갈라진다.
3. `Source/WxGame/Controller/WxPlayerController.h` — 인벤토리 소유·HUD/데스스크린 수명 등 플레이어 세션 접착 로직.
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — 게임↔UI 경계를 잇는 리졸버 패턴의 대표 예.

## 관련
- 상위: 도메인 시스템은 각 플러그인에 위임 — [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxSave]], [[WxSound]], [[WxCore]]

---
*문서 기준 커밋 `1a693b0` · 생성일 2026-07-02 · 소스 48파일 — `/readme-writer`로 갱신*
