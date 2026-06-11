# WxGame — 기본 게임 모듈

> 여러 Wx 플러그인(전투·인벤토리·UI·AI·월드·세이브)을 조립해 실제 플레이 가능한 게임을 만드는 기본 게임 모듈. Framework(GameMode/State/Controller)·구체 캐릭터·월드 오브젝트·입력·MVVM 글루 코드가 여기 모인다.

## 책임
**담당**
- 게임 Framework: `AWxGameMode`(부활 흐름), `AWxGameState`(글로벌 TimeDilation 컴포넌트 호스팅), `AWxPlayerController`, `AWxPlayerState`
- 구체 캐릭터 계층: `AWxCharacterBase`(ASC 직접 소유) → `AWxPlayerCharacter` / `AWxEnemyCharacter` → `AWxBossCharacter`
- 에너미 AI 컨트롤러(`AWxEnemyController`)와 BehaviorTree 실행 연결
- 게임플레이 입력 설정(Enhanced Input + InputTag 매핑 DataAsset)과 캐릭터 입력 바인딩. 메뉴/UI 토글 입력은 CommonUI 액션([[WxUI]])이 소유
- 구체 월드 오브젝트(체크포인트·보물상자·아이템 픽업·레이저 트랩) — [[WxWorld]]의 상호작용/Spawnable을 활용해 구현
- 캐릭터 부착 컴포넌트(장비 외형/EquipEffect GE, 네임플레이트)와 게임 특화 어빌리티(`UWxAbility_Interact`, `UWxAbility_UseItem`) 및 짝 AnimNotify(발소리, Event.UseItem 송출)
- MVVM 글루: 글로벌 뷰모델 등록(`UWxGlobalViewModelSubsystem`), Inventory/Item 뷰모델

**경계 (비담당)**
- 전투/어트리뷰트/어빌리티 베이스 구현은 [[WxCombat]] (ASC·AbilitySet·LockOn 등)
- 인벤토리 매니저/아이템 정의/Fragment는 [[WxInventory]]
- UI 위젯·CommonUI 레이어·ActivatableWidget은 [[WxUI]]
- AI 인식/Blackboard 동기화는 [[WxAI]]의 `UWxAIPerceptionComponent`
- 상호작용 컴포넌트·Spawnable·월드 기믹 베이스는 [[WxWorld]]
- 세이브/로드·월드 상태 복원은 [[WxSave]]
- 공용 정의(태그/Enum/팀 타입)는 [[WxCore]]

## 의존성
- **주요 의존**: `WxCore`, `WxCombat`, `WxInventory`, `WxUI`, `WxWorld`, `WxAI`, `WxSave` + GameplayAbilities(GAS), ModelViewViewModel(MVVM), CommonUI, AIModule, EnhancedInput, MotionWarping, Niagara
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할 — 「WxCore 외 참조 금지」 규칙은 플러그인 전용이라 무관(해당없음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 모든 캐릭터의 공통 골격 — ASC/AttributeSet/LockOn/장비/사망 흐름이 여기서 출발 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력(이동/시선/어빌리티)의 실소유자, `UWxInputConfig`를 소비 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | 인벤토리 소유(소유 클라 단위 복제)·HUD 푸시·뷰모델 Init/Deinit·사망 화면을 잇는 글루 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxEnemyController` | OnPossess에서 BB 키 세팅 + BT 실행; 감지→BB 동기화는 WxAI 컴포넌트에 위임 | `Source/WxGame/Controller/WxEnemyController.h` |
| `AWxGameMode` | 세이브 슬롯의 부활 Transform을 ChoosePlayerStart로 연결([[WxSave]]와 접점) | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxInputConfig` | IMC + Move/Look/Jump + InputAction→InputTag 매핑 DataAsset (입력의 데이터 주도 지점) | `Source/WxGame/Input/WxInputConfig.h` |
| `UWxGlobalViewModelSubsystem` | 글로벌 Shell 뷰모델(Player/Boss/Inventory)을 선등록, 데이터 소스가 내부만 갱신 | `Source/WxGame/MVVM/WxGlobalViewModelSubsystem.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 복제·외형 갱신·EquipEffect GE 라이프사이클([[WxInventory]] 인터페이스 구현) | `Source/WxGame/Component/WxEquipmentComponent.h` |

## 확장 포인트 / 규약
- 새 캐릭터는 `AWxCharacterBase`(또는 Player/Enemy/Boss)를 상속한다. 스탯/어빌리티는 GAS 경로(AbilitySet)로 주입되며 리스폰 시 재초기화된다(PlayerState에 스탯 없음 — ASC를 캐릭터가 직접 소유).
- 플레이어 입력 추가는 `UWxInputConfig` DataAsset에 InputAction→InputTag 항목을 더하는 식으로 데이터 주도. 메뉴/UI 입력은 CommonUI 액션([[WxUI]])으로 처리하고 여기 두지 않는다.
- 새 어빌리티는 [[WxCombat]]의 어빌리티 베이스를 상속한다. 몽타주 시점 동기화가 필요하면 짝 AnimNotify를 `Source/WxGame/AnimNotify/`에 두고 GameplayEvent로 연결한다(예: `UWxAbility_UseItem` ↔ `UWxAnimNotify_UseItem`의 Event.UseItem).
- 새 월드 오브젝트는 [[WxWorld]]의 상호작용 컴포넌트로 노출한다. `Source/WxGame/WorldObject/`의 기존 구현(CheckPoint/TreasureChest/ItemPickup/LaserCorridor)을 본보기로.
- 글로벌 UI 데이터 소스(보스 체력바 등)는 `UWxGlobalViewModelSubsystem`의 Shell 뷰모델 Initialize/Deinitialize로 연동한다 — 뷰모델 인스턴스 자체를 갈아끼우지 않는다(`AWxBossCharacter`가 State.Recognized 관찰로 구동).
- 리플리케이션(최대 4인): 인벤토리는 `AWxPlayerController`에서 소유 클라이언트로만 복제, 장비 GE·아이템 지급(`AWxItemPickup::SetItemDef`)·소비는 서버 권한 측에서 수행.

## 여기서부터 읽어라
1. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터 공통 골격과 GAS 초기화 흐름의 출발점
2. `Source/WxGame/Controller/WxPlayerController.h` — 인벤토리·HUD·뷰모델·사망 화면이 어디서 묶이는지 전체 그림
3. `Source/WxGame/WxGame.Build.cs` — 이 모듈이 조립하는 플러그인 의존 목록

## 관련
- 하위(사용): [[WxCore]], [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxSave]]

---
*문서 기준 커밋 `5ae4876` · 생성일 2026-06-11 · 소스 44파일 — `/readme-writer`로 갱신*
