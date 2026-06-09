# WxGame — 기본 게임 모듈

> 여러 Wx 플러그인(전투·인벤토리·UI·AI·월드·세이브)을 조립해 실제 플레이 가능한 게임 컨텐츠를 만드는 기본 게임 모듈. Framework(GameMode/State/Controller)·구체 캐릭터·월드 오브젝트·입력·MVVM 글루 코드가 여기 모인다.

## 책임
**담당**
- 게임 Framework: `AWxGameMode`(부활 흐름), `AWxGameState`(글로벌 TimeDilation), `AWxPlayerController`, `AWxPlayerState`
- 구체 캐릭터 계층: `AWxCharacterBase`(공통 베이스, ASC 직접 소유) → `AWxPlayerCharacter` / `AWxEnemyCharacter` → `AWxBossCharacter`
- 에너미 AI 컨트롤러(`AWxEnemyController`)와 BehaviorTree 실행 연결
- 게임 입력 설정(Enhanced Input + GameplayTag 매핑)과 캐릭터/컨트롤러 입력 분배
- 구체 월드 오브젝트(체크포인트·보물상자·아이템 픽업·레이저 트랩) — `WxWorld`의 `AWxGimmick` 등을 상속해 구현
- 캐릭터 부착 컴포넌트(장비 외형/GE, 네임플레이트)와 상호작용 어빌리티(`UWxAbility_Interact`)
- MVVM 글루: 글로벌 뷰모델 등록(`UWxGlobalViewModelSubsystem`), Inventory/Item 뷰모델

**경계 (비담당)**
- 전투/어트리뷰트/어빌리티 기반 구현은 [[WxCombat]]에 위임 (ASC·AbilitySet·LockOn 등)
- 인벤토리 매니저/아이템 정의/보상 테이블은 [[WxInventory]]
- UI 위젯·CommonUI 레이어·ActivatableWidget은 [[WxUI]]
- AI 인식/Blackboard 동기화는 [[WxAI]]의 `UWxAIPerceptionComponent`
- Gimmick 베이스·상호작용 컴포넌트·EffectZone·Spawner는 [[WxWorld]]
- 세이브/로드·월드 상태 복원은 [[WxSave]]
- 공용 정의(태그/Enum/팀 타입)는 [[WxCore]]

## 의존성
- **주요 의존(Wx 플러그인)**: `WxCore`, `WxCombat`, `WxInventory`, `WxUI`, `WxWorld`, `WxAI`, `WxSave`
- **특징적 엔진 서브시스템**: GameplayAbilities(GAS), ModelViewViewModel(MVVM), CommonUI, AIModule, EnhancedInput, MotionWarping, Niagara
- 규칙: WxGame은 기본 게임 모듈로 여러 Wx 플러그인을 참조해 컨텐츠를 조립하는 것이 정상 역할이다(규칙 준수).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 부활 흐름·ChoosePlayerStart 처리 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | 글로벌 TimeDilation 관리 컴포넌트 호스팅 | `Source/WxGame/Framework/WxGameState.h` |
| `AWxPlayerController` | 입력(UI)·인벤토리·뷰모델·사망 화면 글루 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxPlayerState` | 세션 단위 공유 상태 거주처 | `Source/WxGame/Player/WxPlayerState.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스(ASC 직접 소유) | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 플레이어 캐릭터·게임플레이 입력 소유 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | BehaviorTree 기반 에너미·Spawnable | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxBossCharacter` | 인식 시 글로벌 보스 체력바 뷰모델 활성화 | `Source/WxGame/Character/WxBossCharacter.h` |
| `AWxEnemyController` | 에너미 AI 컨트롤러·BT 실행 | `Source/WxGame/Controller/WxEnemyController.h` |
| `UWxGlobalViewModelSubsystem` | 글로벌 Shell 뷰모델 등록/조회 | `Source/WxGame/MVVM/WxGlobalViewModelSubsystem.h` |
| `UWxEquipmentComponent` | 장비 외형/EquipEffect GE 라이프사이클 | `Source/WxGame/Component/WxEquipmentComponent.h` |
| `UWxAbility_Interact` | 최근접 상호작용 컴포넌트 대상 단발 어빌리티 | `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h` |

## 폴더 구성
- `Framework/` — GameMode/GameState
- `Controller/` — Player/Enemy 컨트롤러
- `Character/` — 공통 베이스 + Player/Enemy/Boss 구체 캐릭터
- `Player/` — PlayerState
- `Component/` — 캐릭터 부착 컴포넌트(장비, 네임플레이트)
- `AbilitySystem/Ability/` — 게임 컨텐츠용 구체 어빌리티(상호작용)
- `AnimNotify/` — 애님 노티파이(발소리 등)
- `Input/` — 캐릭터/컨트롤러 입력 설정 DataAsset
- `MVVM/` — 글로벌 뷰모델 서브시스템 및 인벤토리/아이템 뷰모델
- `WorldObject/` — 구체 월드 오브젝트(체크포인트·보물상자·아이템 픽업·레이저 트랩)

## 확장 포인트 / 규약
- 새 캐릭터는 `AWxCharacterBase`(또는 Player/Enemy/Boss)를 상속하고, ASC/AttributeSet/AbilitySet은 GAS 경로로 주입한다. 입력은 `UWxCharacterInputConfig`(IMC + Move/Look + InputTag→어빌리티) DataAsset으로 데이터 주도 구성한다.
- 새 월드 오브젝트는 [[WxWorld]]의 `AWxGimmick`을 상속하고 `UWxInteractionComponent`로 상호작용을 노출한다. 발동 상태는 Gimmick의 `bTriggered`/`ApplyState`로 보존된다.
- 새 어빌리티는 [[WxCombat]]의 `UWxAbilityBase`를 상속한다(예: `UWxAbility_Interact`).
- 보스 등 글로벌 UI 데이터 소스는 `UWxGlobalViewModelSubsystem`의 Shell 뷰모델 Initialize/Deinitialize로 연동한다.
- 리플리케이션/권한(최대 4인 멀티): 캐릭터가 ASC를 직접 소유하며 리스폰 시 스탯 재초기화. 인벤토리는 `AWxPlayerController`가 소유 클라이언트 단위로만 복제. 장비/픽업의 GE·아이템 지급은 서버 권한 측에서 수행한다.

## 여기서부터 읽어라
1. `Source/WxGame/Character/WxCharacterBase.h` — 모든 캐릭터의 공통 골격(ASC 소유·팀·장비·사망 흐름)의 진입점
2. `Source/WxGame/Controller/WxPlayerController.h` — 입력·인벤토리·UI·뷰모델을 잇는 플레이어측 글루의 전체 그림
3. `Source/WxGame/WxGame.Build.cs` — 이 모듈이 조립하는 Wx 플러그인 의존 목록

## 관련
- 하위(사용): [[WxCore]], [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxSave]]

---
*문서 기준 커밋 `80cc348` · 생성일 2026-06-09 · 소스 46파일 — `/readme-writer`로 갱신*
