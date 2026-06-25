# WxGame — 기본 게임 모듈 (프레임워크 조립)

> 여러 Wx 도메인 플러그인(Combat/Inventory/UI/World/AI/Save)을 조립해 실제로 플레이 가능한 게임을 구성하는 런타임 소스 모듈이다. GameFramework 진입점(GameMode/GameState/Controller/Character)과 구체 월드 오브젝트가 여기에 산다.

## 책임
**담당**
- GameFramework 골격: GameMode/GameState/PlayerController/PlayerState (`Framework/`, `Controller/`, `Player/`)
- 캐릭터 계층: 공통 베이스(`AWxCharacterBase`) → 구체 Player/Enemy/Boss (`Character/`)
- 플레이어 입력 → GAS 어빌리티/이동 연결: Enhanced Input + `UWxInputConfig` (`Input/`)
- 게임 고유 어빌리티/타깃데이터/애님노티파이 (`AbilitySystem/`, `AnimNotify/`)
- 구체 월드 오브젝트(체크포인트, 레이저 트랩 등) (`WorldObject/`)
- WxUI ViewModel에 게임 런타임 데이터를 주입하는 Resolver/VM 어댑터 (`MVVM/`)

**경계 (비담당)**
- GAS 코어·전투 규칙·어트리뷰트 → [[WxCombat]]
- AI Perception/BT 태스크·정찰 등 행동 인프라 → [[WxAI]]
- 인벤토리 데이터/매니저 로직 → [[WxInventory]]
- 위젯/MVVM ViewModel 정의·CommonUI 레이아웃 → [[WxUI]]
- 상호작용·기믹(`AWxGimmick`)·스폰 인프라 → [[WxWorld]]
- 세이브/로드 서브시스템·월드 상태 복원 → [[WxSave]]
- 공용 정의(팀 타입 등) → [[WxCore]]

## 의존성
- **주요 의존**: `WxCore`, `WxCombat`, `WxInventory`, `WxUI`, `WxWorld`, `WxAI`, `WxSave` · 엔진: `GameplayAbilities`/`GameplayTags`, `ModularGameplay`, `EnhancedInput`, `ModelViewViewModel`, `CommonUI`, `MotionWarping`, `AIModule`, `Niagara`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | `ChoosePlayerStart`를 `UWxPlayerSpawningComponent`에 위임, 프레임워크 컴포넌트를 GameState에 주입 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxPlayerSpawningComponent` | 스폰 지점 선택 로직 소유(저장 태그 → "Default" 태그). Lyra 패턴 GameState 컴포넌트 | `Source/WxGame/Framework/WxPlayerSpawningComponent.h` |
| `AWxCharacterBase` | ASC를 직접 소유하는 Player/Enemy 공통 베이스. 팀/사망/장비/이동속도 처리의 중심 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라 + 입력 소유. `UWxInputConfig`로 어빌리티 바인딩 주입 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyController` | OnPossess에서 BB 세팅 + BT 실행. Perception은 컴포넌트에 위임 | `Source/WxGame/Controller/WxEnemyController.h` |
| `AWxPlayerController` | 인벤토리 매니저를 소유 클라 단위로 보유, HUD/데스스크린 푸시 | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxInputConfig` | IMC + Move/Look + 어빌리티 InputAction↔GameplayTag 매핑 DataAsset | `Source/WxGame/Input/WxInputConfig.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 Pawn 데이터를 WxUI ViewModel에 주입하는 MVVM 어댑터 | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **프레임워크 컴포넌트 주입**: `AWxGameMode::FrameworkComponents`(EditDefaults)에 `UGameFrameworkComponent`를 등록하면 ModularGameplay가 receiver(GameState 등)에 자동 주입한다. GameState는 어떤 컴포넌트가 붙는지 알지 않는다.
- **스폰 지점**: 선택 로직은 GameMode가 아니라 `UWxPlayerSpawningComponent`가 소유(저장 PlayerStartTag → "Default" 태그 → 엔진 폴백). `AWxCheckPoint`(APlayerStart 파생)가 부활 후보가 된다.
- **새 적 종류**: `AWxEnemyCharacter` 파생 BP에서 `BehaviorTreeAsset`/시야·청각/`RewardRow`(+`LaunchSpeed`)를 지정. 사망 처리는 `HandleDeath` override로 확장.
- **새 플레이어 입력/어빌리티**: `UWxInputConfig` DataAsset에 `AbilityInputBindings`(InputAction→InputTag) 추가 — C++ 수정 없이 데이터 주도. 메뉴/UI 입력은 CommonUI(`WxHUDLayout`)가 별도 소유.
- **MVVM 가교**: WxUI ViewModel은 게임/World 모듈을 참조할 수 없으므로, 양쪽에 의존하는 `WxViewModelResolver_*`가 데이터를 시드한다. WBP에서 Creation Type = Resolver로 선택.
- **리플리케이션/권한**: ASC는 캐릭터가 직접 소유(PlayerState 아님). 인벤토리는 PlayerController가 소유 연결로만 복제(다른 클라 미복제). 기믹 State는 C++가 권위 소유, 클라는 복제 State를 StateTree가 추종(`AWxLaserCorridor` 참고).

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 스폰 위임 + 프레임워크 컴포넌트 주입. 모듈이 플러그인을 조립하는 출발점.
2. `Source/WxGame/Character/WxCharacterBase.h` — ASC 소유·팀·사망·장비의 허브. 캐릭터 계층 전체의 기준점.
3. `Source/WxGame/Controller/WxPlayerController.h` — HUD/인벤토리/데스스크린 등 플레이어 세션 흐름의 조립 지점.
4. `Source/WxGame/Character/WxPlayerCharacter.h` + `Source/WxGame/Input/WxInputConfig.h` — 입력→어빌리티 데이터 주도 연결.

## 관련
- 상위: 게임의 최상위 조립 모듈. 모든 Wx 도메인 플러그인([[WxCombat]] · [[WxAI]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxSave]] · [[WxCore]])을 소비한다.

---
*문서 기준 커밋 `1735fc7` · 생성일 2026-06-25 · 소스 50파일 — `/readme-writer`로 갱신*
