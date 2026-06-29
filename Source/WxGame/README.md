# WxGame — 게임 모듈 (플러그인 조립 + 프레임워크)

> 플레이어/적/보스 캐릭터, GAS 기반 입력·어빌리티, 게임 프레임워크(GameMode/State/Controller), 월드 오브젝트, 그리고 WxUI↔도메인 플러그인을 잇는 MVVM 리졸버를 한데 모으는 기본 게임 모듈. 각 도메인 플러그인(WxCombat/WxInventory/WxAI/WxWorld/WxUI/WxSave)을 실제 게임플레이로 조립하는 자리다.

## 책임
**담당**
- 캐릭터 계층(`AWxCharacterBase` → Player/Enemy/Boss)과 ASC 직접 소유·초기화, SPD→이동속도 반영, 사망/팀 처리
- 게임 프레임워크: GameMode(스폰 위임 + ModularGameplay 컴포넌트 주입), GameState(컴포넌트 receiver), PlayerController(인벤토리 소유·HUD/데스스크린 푸시), PlayerState
- 플레이어 입력: EnhancedInput IMC + 어빌리티 InputTag 매핑(`UWxInputConfig`), Move/Look, 어빌리티 입력 트리거
- 게임 모듈 고유 어빌리티: 상호작용(`UWxAbility_Interact`), 소비 아이템 사용(`UWxAbility_UseItem`)과 그 AnimNotify
- 월드 오브젝트: 체크포인트(`AWxCheckPoint`), 레이저 트랩(`AWxLaserCorridor`)
- WxUI 위젯과 도메인 플러그인 데이터를 잇는 MVVM 리졸버/뷰모델(플러그인 간 의존 금지를 양쪽 의존 모듈에서 해소)

**경계 (비담당)**
- 전투 규칙·어트리뷰트·`UWxAbilityBase`·무기 → [[WxCombat]]
- 인벤토리 데이터/아이템 정의·보상 지급 → [[WxInventory]]
- AI 인지·행동트리·정찰·스포너 → [[WxAI]]
- 상호작용 컴포넌트/레지스트리·기믹·StateTree 인프라 → [[WxWorld]]
- 위젯·뷰모델 베이스(`UWxViewModel_Character` 등)·CommonUI 레이아웃 → [[WxUI]]
- savable 액터 상태 복원 → [[WxSave]]

## 의존성
- **주요 의존**: `WxCore`, `WxCombat`, `WxInventory`, `WxAI`, `WxWorld`, `WxUI`, `WxSave` / 엔진: `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `ModularGameplay`, `ModelViewViewModel`, `CommonUI`, `EnhancedInput`(Private), `AIModule`, `MotionWarping`, `Niagara`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 모든 캐릭터의 베이스. ASC를 직접 소유하고 사망/팀/장비 비주얼을 처리. 아래 모든 캐릭터가 여기서 갈라진다 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 플레이어 폰. 카메라·락온·입력·상호작용 위젯. `UWxInputConfig`로 입력 주입 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 적 폰. BT/시야·청각 파라미터 보유, 처치 시 [[WxInventory]] 보상 지급, [[WxAI]] Spawner 연동 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 인벤토리 소유처(소유 클라 복제), HUD/데스스크린 푸시, 캐릭터 사망 바인딩 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxGameMode` | 스폰 선택을 `UWxPlayerSpawningComponent`에 위임, ModularGameplay 프레임워크 컴포넌트 주입 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxInputConfig` | IMC + Move/Look/Jump + 어빌리티 InputTag 매핑 DataAsset | `Source/WxGame/Input/WxInputConfig.h` |
| `UWxAbility_Interact` | 상호작용 어빌리티(상주 스캔 + 입력 트리거, 로컬 선택→서버 실행) | `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h` |
| `UWxViewModelResolver_PlayerCharacter` | WBP View Bindings 리졸버 패턴의 대표. WxUI↔도메인 데이터 주입을 양쪽 의존 모듈이 수행 | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **새 캐릭터**: `AWxCharacterBase`(Abstract) 또는 `AWxEnemyCharacter`/`AWxBossCharacter`를 BP로 상속. ASC는 캐릭터가 직접 소유하며 리스폰 시 재초기화(PlayerState 불필요). 무기는 `WeaponActor`(ChildActor)에 구체 무기 BP를 지정, 적의 BT/보상/시야는 BP 디폴트로 설정.
- **새 어빌리티**: [[WxCombat]]의 `UWxAbilityBase` 상속. 입력 트리거형은 `UWxInputConfig.AbilityInputBindings`에 InputAction↔InputTag를 추가하면 `AWxPlayerCharacter`가 자동 바인딩.
- **새 월드 오브젝트**: 즉시·반복형(모닥불 류)은 `AWxCheckPoint`처럼 단순 상속(APlayerStart 파생이라 부활 지점 후보로도 잡힘), 영속 State/StateTree 기믹은 [[WxWorld]]의 `AWxGimmick` 상속(예: `AWxLaserCorridor`).
- **새 프레임워크 컴포넌트**: `UGameStateComponent` 파생 후 `AWxGameMode.FrameworkComponents` 목록에 등록 → `InitGame`에서 ModularGameplay 매니저가 GameState에 자동 주입(GameState 수정 불필요, 예: `UWxPlayerSpawningComponent`).
- **WxUI 위젯 연동**: WBP View Bindings에서 Creation Type=Resolver로 `MVVM/`의 `UWxViewModelResolver_*`를 선택. 도메인 플러그인이 WxUI를 모르고 WxUI가 도메인을 모르므로, 양쪽에 의존하는 본 모듈의 리졸버가 데이터를 주입한다.
- **리플리케이션**: 상호작용 선택은 로컬 레지스트리 소유 → 입력 시 클라가 읽어 서버로 TargetData 전송, 실행은 서버 권한. 인벤토리는 PlayerController가 소유 클라 연결로만 복제.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 스폰 위임(Lyra 패턴)과 ModularGameplay 컴포넌트 주입으로 부팅 프레임워크가 어떻게 조립되는지
2. `Source/WxGame/Character/WxCharacterBase.h` — 모든 캐릭터의 ASC 소유·초기화·사망/팀 모델. 캐릭터 계층의 뿌리
3. `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h` — 상호작용의 클라 감지/선택과 서버 권한 실행 흐름(헤더 주석에 전체 분기 정리)
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — 리졸버 패턴과 도메인↔WxUI 데이터 주입 규약의 대표 사례

## 관련
- 상위: 게임 전체의 조립점(이 위에는 엔진뿐). 도메인 로직은 [[WxCombat]] · [[WxInventory]] · [[WxAI]] · [[WxWorld]] · [[WxUI]] · [[WxSave]] · 공용 정의는 [[WxCore]]

---
*문서 기준 커밋 `97577fb` · 생성일 2026-06-29 · 소스 48파일 — `/readme-writer`로 갱신*
