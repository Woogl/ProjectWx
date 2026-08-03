# WxGame — 기본 게임 모듈

> 여러 Wx 플러그인을 조립하는 런타임 게임 모듈. GameMode/GameState/Controller 프레임워크와 캐릭터 베이스를 직접 소유하고, 전투·인벤토리·AI·UI·월드·세이브·대화·퀘스트는 각 플러그인에 위임해 하나의 플레이 가능한 게임으로 엮는다.

## 책임
**담당**
- 게임플레이 프레임워크 구체 클래스: `AWxGameMode`/`AWxGameState`/`AWxPlayerState`, `AWxPlayerController`/`AWxEnemyController`, `AWxCharacterBase`/`AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`
- 캐릭터에 ASC를 직접 소유시키고(리스폰 시 스탯 재초기화) 어트리뷰트→이동속도·사망·래그돌 태그 반응을 배선
- 게임 모듈 고유 어빌리티(`WxAbility_Interact`/`WxAbility_UseItem`)와 그 AnimNotify(`WxAnimNotify_UseItem`)
- 프레임워크 액터를 ModularGameplay receiver로 opt-in시키고(컨트롤러·캐릭터·PlayerState·GameState), Experience 액션이 플러그인 컴포넌트(인벤토리/상호작용 스캐너/대화 세션/PlayerSpawn 등)를 데이터 주도로 주입하게 한다. 캐릭터는 전투·장비·모션워핑 컴포넌트를 직접 조립한다
- 플러그인 데이터를 WxUI 뷰모델에 주입하는 MVVM 리졸버·브리지 뷰모델(`MVVM/`)
- 게임 고유 프레임워크 파생물: 비대칭 중력 이동(`UWxCharacterMovementComponent`), 개발용 콘솔 치트(`UWxCheatManager`), PIE 다중 세션 GF 활성 카운팅(`UWxExperienceManager` 엔진 서브시스템)

**경계 (비담당)**
- ASC/AttributeSet/무기/락온/처형 규칙·어빌리티 베이스 정의 — [[WxCombat]] (본 모듈은 컴포넌트를 조립·소유만)
- 위젯·뷰모델 베이스·HUDLayout 등 UI 프레임워크 — [[WxUI]]
- 상호작용 스캐너·기믹/StateTree 인프라·Spawner — [[WxWorld]]
- 인벤토리 자료구조·보상 지급(`GrantReward`) — [[WxInventory]], 대화 진행 — [[WxDialogue]], 퀘스트 저널 — [[WxQuest]]
- AI 지각·Blackboard·BT Task·정찰 — [[WxAI]], 저장/영속 복원 — [[WxSave]], 팀·공용 정의 — [[WxCore]]

## 의존성
- **주요 의존**: `WxCore` `WxCombat` `WxUI` `WxWorld` `WxInventory` `WxDialogue` `WxQuest` `WxAI` `WxSave`(Public) + `EnhancedInput`(Private)
- **엔진 서브시스템**: `GameplayAbilities`/`GameplayTags`/`GameplayTasks`, `ModelViewViewModel`(MVVM), `ModularGameplay`(프레임워크 컴포넌트 주입), `GameFeatures`(Experience 액션·GF 플러그인 활성), `MotionWarping`, `AIModule`, `CommonUI`, `UMG`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 게임 골격 진입점. 진입 URL(`?Experience=`)→WorldSettings→자체 폴백 순으로 Experience를 확정해 GameState의 매니저에 넘기고, 폰 스폰·시작 지급을 로드 완료까지 미룬다. 폰 클래스는 Experience의 `DefaultPawnClass`가 정한다(상속받은 `DefaultPawnClass`는 읽지 않음) | `Framework/WxGameMode.h` |
| `UWxExperienceManagerComponent` | Experience 로드·적용의 주체(GameState 서브오브젝트). 참조 복제 후 서버·클라 각자 번들 비동기 로드→GF 플러그인 활성→액션 실행→`CallOrRegister_OnExperienceLoaded` 브로드캐스트 | `Framework/WxExperienceManagerComponent.h` |
| `AWxCharacterBase` | 플레이어·적 공통 Abstract 베이스. ASC/AttributeSet/장비/모션워핑 직접 소유, 팀·사망·SPD 이동 반영 | `Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라 + Enhanced Input + 어빌리티 입력·락온·상호작용 위젯 소유 | `Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | BT 구동 적(Abstract). 시야/청각·처형 어포던스(앞잡/뒤잡)·보상 지급·스포너 연동, `AWxBossCharacter`가 파생 | `Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 순수 ModularGameplay receiver. 주입된 컨트롤러 컴포넌트(인벤토리·상호작용 스캐너·대화 세션·PlayerSpawn)를 자동 부착만 하고 중개하지 않으며, 화면 push는 [[WxUI]] `UWxUIManagerSubsystem`이 담당 | `Controller/WxPlayerController.h` |
| `AWxEnemyController` | 폰 BT 실행·BB 컨텍스트 키 세팅, Perception→BB 동기화는 `UWxAIPerceptionComponent`에 위임 | `Controller/WxEnemyController.h` |
| `UWxInputConfig` | IMC + Move/Look/Jump/Crouch 직접 바인딩 입력 DataAsset(어빌리티 입력은 담지 않음) | `Input/WxInputConfig.h` |
| `UWxViewModelResolver_PlayerCharacter` | 폰 ASC/표시 데이터를 WxUI 뷰모델에 주입하는 리졸버(MVVM 글루 대표) | `MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- 새 캐릭터/적/보스는 `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`를 BP 상속 후 컴포넌트(무기 `ChildActorClass`, `BehaviorTreeAsset`, `RewardRow` 등)를 디폴트에서 지정. ASC는 PlayerState가 아닌 캐릭터가 직접 소유(리스폰 시 스탯 재초기화). 보스는 `AWxBossCharacter`만 상속하면 `UWxViewModel_BossCharacter`가 스폰/EndPlay를 관찰해 체력바를 붙인다(클래스 내 UI 코드 없음).
- 직접 바인딩 입력(이동/시선/점프/웅크리기)은 `UWxInputConfig`에 IA를 추가하고 `AWxPlayerCharacter::SetupPlayerInputComponent`에서 바인딩. 어빌리티 입력은 여기 두지 않고 AbilitySet 부여 대상 CDO에서 파생해 자동 바인딩. 상호작용은 HUD 리스트 위젯이 Enhanced Input으로 직접 받고, 메뉴/UI 입력은 CommonUI 액션([[WxUI]] `WxHUDLayout`)으로 처리.
- 새 어빌리티는 [[WxCombat]] `UWxAbilityBase` 파생. 게임 모듈 고유 실행(상호작용/아이템 사용)은 본 모듈에, 전투 공용 로직은 WxCombat에 둔다.
- Experience(`UWxExperienceDefinition`, `/Game/Framework` 스캔)는 GameFeature 플러그인 목록 + 액션(`GameFeatureAction`) + 액션셋(`UWxExperienceActionSet`) + 폰 클래스 + 시작 아이템으로 게임 구성을 데이터화한다. 시작 아이템은 참조한 액션셋 목록의 합산이며(본체엔 필드가 없다) 전용 액션셋 `WAS_StartingItems`(튜닝 영역, 배선인 `WAS_CoreGameplay`와 분리)에 담는다. 프레임워크 컴포넌트 주입은 자체 `Add Components` 액션(`UWxGameFeatureAction_AddComponents`)에 엔트리(컴포넌트 클래스만 — 대상 액터는 지정하지 않는다)를 추가하면, 대상은 그 컴포넌트가 상속한 프레임워크 베이스(Pawn/Controller/PlayerState/GameState 컴포넌트)에서 도출돼 receiver(GameState/PlayerController/PlayerState/CharacterBase — 각 클래스가 수동 opt-in)에 자동 부착된다. 공용 6종은 `WAS_CoreGameplay`에 있다.
- 주입 엔트리에 클라·서버 사이드 플래그는 없다 — 넷모드와 무관하게 양측에 요청되고, 사이드 제한은 **컴포넌트 스스로** 한다(권위 전용은 `HasAuthority` 가드, 로컬 표시 전용은 `IsLocalController` 가드). 복제 컴포넌트만 엔진이 authority로 제한한다.
- 미니게임·사이드미션 같은 탈부착 콘텐츠는 `Plugins/GameFeatures/`의 GF 플러그인(초기 상태 Registered)으로 패키징하고, 그걸 켜는 Experience 에셋의 `GameFeaturesToEnable`에 이름을 적는다. GF 플러그인은 DAG 최상단이라 WxGame·도메인 참조 가능, 역참조 금지. 이름은 GF 표식 없이 `Wx`+콘텐츠명으로 짓는다. 축 전체는 2026-07-29 샘플로 실증 후 정리됐다(절차·함정은 워크로그 참고).
- 로드는 비동기다 — 로드 완료에 의존하는 초기화는 `CallOrRegister_OnExperienceLoaded`로 대기하고, 주입 컴포넌트가 로그인 이벤트에 의존하면 부착이 로그인보다 늦은 경우의 캐치업을 자기 `OnRegister`에 마련한다(예: WxSave `UWxPlayerSpawnComponent`).
- 새 월드 오브젝트/기믹은 [[WxWorld]] `UWxGimmickStateTreeComponent` 를 붙인 BP 액터로 만든다(C++ 불필요). 상태·전이·연출이 전부 ST 에셋에 있고, 컴포넌트는 상호작용 이벤트 발행과 상태 Tag 영속만 맡는다.
- 개발용 치트는 `UWxCheatManager`(`Cheat/`, Exec 함수)에 추가한다 — Standalone·에디터에서만 생성되고 그때가 곧 권위 측이라 권위 가드 없이 정상 대미지·사망 파이프라인을 그대로 탄다. 배포 빌드엔 존재하지 않는다.
- MVVM 글루: WxUI 뷰모델이 게임 모듈을 참조할 수 없으므로, 양쪽에 의존하는 리졸버·브리지 뷰모델(`MVVM/`)이 플러그인 데이터를 위젯에 잇는다. WBP의 View Bindings에서 Creation Type=Resolver로 선택하면 유일한 주입 통로가 된다.

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — 게임 부팅(Experience 선택·시작 아이템)의 조립 지점. 재개/스탯 복원 위임 경계 파악.
2. `Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트(ASC/장비/모션워핑)를 조립하는지 = 모듈 경계의 축소판.
3. `Controller/WxPlayerController.h` — 인벤토리 소유 위치와 상호작용/대화/HUD 흐름.
4. `WxGame.Build.cs` — 이 모듈이 조립하는 플러그인·엔진 서브시스템 전체 목록.

## 관련
- 상위: 조립하는 플러그인 [[WxCombat]] · [[WxUI]] · [[WxWorld]] · [[WxInventory]] · [[WxDialogue]] · [[WxQuest]] · [[WxAI]] · [[WxSave]] · [[WxCore]]

---
*문서 기준 커밋 `28ee2c6` · 생성일 2026-08-03 · 소스 60파일 — `/readme-writer`로 갱신*
