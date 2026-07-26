# WxGame — 기본 게임 모듈

> 여러 Wx 플러그인을 조립하는 런타임 게임 모듈. GameMode/GameState/Controller 프레임워크와 캐릭터 베이스를 직접 소유하고, 전투·인벤토리·AI·UI·월드·사운드·세이브·대화·퀘스트는 각 플러그인에 위임해 하나의 플레이 가능한 게임으로 엮는다.

## 책임
**담당**
- 게임플레이 프레임워크 구체 클래스: `AWxGameMode`/`AWxGameState`/`AWxPlayerState`, `AWxPlayerController`/`AWxEnemyController`, `AWxCharacterBase`/`AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`
- 캐릭터에 ASC를 직접 소유시키고(리스폰 시 스탯 재초기화) 어트리뷰트→이동속도·사망·래그돌 태그 반응을 배선
- 게임 모듈 고유 어빌리티(`WxAbility_Interact`/`WxAbility_UseItem`)와 그 AnimNotify(`WxAnimNotify_UseItem`)
- 플러그인 컴포넌트를 컨트롤러·캐릭터에 부착·배선(인벤토리/상호작용 스캐너/대화 세션/락온/BGM/네임플레이트 등)
- 플러그인 데이터를 WxUI 뷰모델에 주입하는 MVVM 리졸버·브리지 뷰모델(`MVVM/`)
- 게임 고유 월드 오브젝트 `AWxLaserCorridor`(WxWorld `AWxGimmick` 파생 트랩)

**경계 (비담당)**
- ASC/AttributeSet/무기/락온/처형 규칙·어빌리티 베이스 정의 — [[WxCombat]] (본 모듈은 컴포넌트를 조립·소유만)
- 위젯·뷰모델 베이스·HUDLayout 등 UI 프레임워크 — [[WxUI]]
- 상호작용 스캐너·기믹/StateTree 인프라·Spawner — [[WxWorld]]
- 인벤토리 자료구조·보상 지급(`GrantReward`) — [[WxInventory]], 대화 진행 — [[WxDialogue]], 퀘스트 저널 — [[WxQuest]]
- AI 지각·Blackboard·BT Task·정찰 — [[WxAI]], 저장/영속 복원 — [[WxSave]], BGM 소스/Chooser — [[WxSound]], 팀·공용 정의 — [[WxCore]]

## 의존성
- **주요 의존**: `WxCore` `WxCombat` `WxUI` `WxWorld` `WxInventory` `WxDialogue` `WxQuest` `WxAI` `WxSave`(Public) + `WxSound` `EnhancedInput`(Private)
- **엔진 서브시스템**: `GameplayAbilities`/`GameplayTags`/`GameplayTasks`, `ModelViewViewModel`(MVVM), `ModularGameplay`(프레임워크 컴포넌트 주입), `MotionWarping`, `AIModule`, `CommonUI`, `UMG`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 게임 골격 진입점. `InitGame`에서 프레임워크 컴포넌트를 요청 등록→receiver 자동 주입. 플레이어 스폰은 엔진 기본 경로에 위임 | `Framework/WxGameMode.h` |
| `AWxCharacterBase` | 플레이어·적 공통 Abstract 베이스. ASC/AttributeSet/장비/모션워핑 직접 소유, 팀·사망·SPD 이동 반영 | `Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라 + Enhanced Input + 어빌리티 입력·락온·상호작용 위젯·BGM 소유 | `Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | BT 구동 적(Abstract). 시야/청각·처형 어포던스(앞잡/뒤잡)·보상 지급·스포너 연동, `AWxBossCharacter`가 파생 | `Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 인벤토리/상호작용 스캐너/대화 세션 소유, HUD·사망화면 push, ModularGameplay receiver | `Controller/WxPlayerController.h` |
| `AWxEnemyController` | 폰 BT 실행·BB 컨텍스트 키 세팅, Perception→BB 동기화는 `UWxAIPerceptionComponent`에 위임 | `Controller/WxEnemyController.h` |
| `UWxInputConfig` | IMC + Move/Look/Jump/Crouch 직접 바인딩 입력 DataAsset(어빌리티 입력은 담지 않음) | `Input/WxInputConfig.h` |
| `UWxViewModelResolver_PlayerCharacter` | 폰 ASC/표시 데이터를 WxUI 뷰모델에 주입하는 리졸버(MVVM 글루 대표) | `MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- 새 캐릭터/적/보스는 `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`를 BP 상속 후 컴포넌트(무기 `ChildActorClass`, `BehaviorTreeAsset`, `RewardRow`, BGM 태그 등)를 디폴트에서 지정. ASC는 PlayerState가 아닌 캐릭터가 직접 소유(리스폰 시 스탯 재초기화). 보스는 `AWxBossCharacter`만 상속하면 `UWxViewModel_BossCharacter`가 스폰/EndPlay를 관찰해 체력바를 붙인다(클래스 내 UI 코드 없음).
- 직접 바인딩 입력(이동/시선/점프/웅크리기)은 `UWxInputConfig`에 IA를 추가하고 `AWxPlayerCharacter::SetupPlayerInputComponent`에서 바인딩. 어빌리티 입력은 여기 두지 않고 AbilitySet 부여 대상 CDO에서 파생해 자동 바인딩. 상호작용은 HUD 리스트 위젯이 Enhanced Input으로 직접 받고, 메뉴/UI 입력은 CommonUI 액션([[WxUI]] `WxHUDLayout`)으로 처리.
- 새 어빌리티는 [[WxCombat]] `UWxAbilityBase` 파생. 게임 모듈 고유 실행(상호작용/아이템 사용)은 본 모듈에, 전투 공용 로직은 WxCombat에 둔다.
- GameMode `FrameworkComponents`(EditDefaultsOnly)에 프레임워크 컴포넌트를 추가하면 GameState/Controller 등 receiver에 자동 주입(receiver는 무엇이 붙는지 모른다). 새 프레임워크 기능은 컴포넌트로 추가.
- 새 월드 오브젝트/기믹은 [[WxWorld]] `AWxGimmick` 상속(예: `AWxLaserCorridor`). 권위 State만 C++가 확정·복제하고 비주얼·스폰은 GimmickStateTree가 담당하는 패턴.
- MVVM 글루: WxUI 뷰모델이 게임 모듈을 참조할 수 없으므로, 양쪽에 의존하는 리졸버·브리지 뷰모델(`MVVM/`)이 플러그인 데이터를 위젯에 잇는다. WBP의 View Bindings에서 Creation Type=Resolver로 선택하면 유일한 주입 통로가 된다.

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — 게임 부팅(프레임워크 컴포넌트 주입)의 조립 지점. 재개/스탯 복원 위임 경계 파악.
2. `Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트(ASC/장비/모션워핑)를 조립하는지 = 모듈 경계의 축소판.
3. `Controller/WxPlayerController.h` — 인벤토리 소유 위치와 상호작용/대화/HUD 흐름.
4. `WxGame.Build.cs` — 이 모듈이 조립하는 플러그인·엔진 서브시스템 전체 목록.

## 관련
- 상위: 조립하는 플러그인 [[WxCombat]] · [[WxUI]] · [[WxWorld]] · [[WxInventory]] · [[WxDialogue]] · [[WxQuest]] · [[WxAI]] · [[WxSave]] · [[WxSound]] · [[WxCore]]

---
*문서 기준 커밋 `1bd11a9` · 생성일 2026-07-26 · 소스 46파일 — `/readme-writer`로 갱신*
