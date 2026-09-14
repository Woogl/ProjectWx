# WxGame — 게임 기본 모듈

> 도메인 플러그인(전투·인벤토리·UI·월드·AI·대화·퀘스트)을 실제 폰·컨트롤러·프레임워크 클래스로 조립하는 게임 기본 모듈. 어느 한 도메인에도 속하지 않고 여러 도메인이 만나는 지점(캐릭터 합성, 프론트엔드 흐름, MVVM 배선)을 여기서 잇는다.

## 책임
**담당**
- 프레임워크 조립: `AWxGameMode`/`AWxGameState`/`AWxPlayerState`가 GAS·퀘스트 컴포넌트·프론트엔드 폰 선택을 배선.
- 캐릭터 계층: `AWxCharacterBase`(ASC·모션워핑·락온·히트스톱 소유)와 플레이어/적/NPC 파생.
- 컨트롤러: 플레이어(인벤토리·상호작용·대화·레이아웃 컴포넌트)와 AI(감각·BT 타겟→락온 통로).
- 프론트엔드 흐름: `UWxGameFlowSubsystem`이 선택 폰·목적지 맵을 들고 전환.
- 입력·MVVM·치트: Enhanced Input 설정, 위젯 뷰모델 리졸버, 개발용 Exec 치트.

**경계 (비담당)**
- 전투 로직 → [[WxCombat]] · 아이템/보상 → [[WxInventory]] · HUD/위젯 → [[WxUI]] · 상호작용/스폰 → [[WxWorld]] · AI 판단 → [[WxAI]] · 대화 → [[WxDialogue]] · 퀘스트 → [[WxQuest]] · 공용 정의 → [[WxCore]]. 이 모듈은 각 도메인의 컴포넌트/인터페이스를 폰에 붙여 잇기만 한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 폰/스테이트 클래스 지정, 프론트엔드 선택 폰을 `GetDefaultPawnClassForController`로 반영 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxGameFlowSubsystem` | 프론트엔드 선택(폰·목적지)을 들고 맵 전환, 목적지 월드에서 선택 폰 반환 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |
| `AWxCharacterBase` | 공통 베이스: ASC·전투 컴포넌트·팀·UI 데이터 소유 (플레이어/적 공통) | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력·카메라·아이템사용/입력버퍼 컴포넌트 소유 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 적 AI 조립·상호작용·보상·보스 표시 상태 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxNpc` | 대화 NPC(폰 아닌 액터). 대화×외형 합성 지점 | `Source/WxGame/Character/WxNpc.h` |
| `AWxPlayerController` | 폰 리스폰에도 살아남는 플레이어 단위 컴포넌트(인벤/스캐너/대화/레이아웃) 소유 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxAIController` | AI 폰 공용 컨트롤러: 감각 형태 설정, BT 타겟을 락온으로 옮김 | `Source/WxGame/Controller/WxAIController.h` |

## 확장 포인트 / 규약
- 맵별 GameMode는 BP(GM_FrontEnd·GM_Combat)와 WorldSettings의 GameModeOverride로 고른다. 폰 클래스는 프론트엔드 선택이 우선, 없으면 `DefaultPawnClass`.
- 플레이어 단위 상태(인벤토리·상호작용·대화·레이아웃)는 폰 리스폰에도 유지돼야 하므로 컨트롤러가 소유한다. `AWxPlayerState`는 현재 빈 클래스(스탯은 캐릭터 ASC가 리스폰마다 새로 초기화).
- 캐릭터 외형은 `UWxMetaHumanComponent`에 에셋만 지정하면 등록 시점에 부착물을 조립(`Source/WxGame/Character/Component/WxMetaHumanComponent.h`).
- 입력은 `UWxInputConfig` 데이터에셋으로 주도. 어빌리티/상호작용/메뉴 입력은 여기 두지 않고 각각 AbilitySet·HUD 위젯·CommonUI가 소유.
- 위젯 데이터 바인딩은 `MVVM/` 리졸버(`WxViewModelResolver_Ability` 등)와 뷰모델이 담당.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.cpp` — 프론트엔드 선택→폰 반영, 스테이트 클래스 배선의 시작점.
2. `Source/WxGame/Character/WxCharacterBase.h` — 모든 캐릭터가 무엇을 소유하고 어느 도메인과 잇는지.
3. `Source/WxGame/Controller/WxPlayerController.h` — 플레이어 단위 컴포넌트가 어느 도메인에서 오는지의 지도.
4. `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` — 프론트엔드→게임 맵 전환 흐름. (`Tests/`의 `WxHitStopMovementTest.cpp`는 이동 히트스톱 회귀 테스트.)

## 관련
- 조립 대상 도메인: [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · foundation [[WxCore]]

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 58파일 — `/readme-writer`로 갱신*
