# WxGame — 게임 조립 모듈

> 도메인 플러그인(전투·인벤토리·AI·대화·퀘스트·UI·월드)을 하나의 플레이 가능한 게임으로 조립하는 진입점 모듈. GameMode·Controller·Character 등 프레임워크 골격과, 각 도메인을 잇는 교차 지점(캐릭터 합성, MVVM 브리지)을 소유한다.

## 책임
**담당**
- 게임 프레임워크 골격: GameMode / GameState / PlayerController / PlayerState / AIController
- 캐릭터 계층: 공통 베이스와 플레이어·적·NPC 파생, 그리고 각 캐릭터에 도메인 컴포넌트를 붙이는 합성
- 프론트엔드 → 게임플레이 흐름(폰·목적지 선택, 맵 전환)과 리스폰
- 게임 고유 입력 구성(이동/시선/점프/앉기)과 그 바인딩
- 게임 데이터 ↔ WxUI 뷰모델을 잇는 MVVM 리졸버·뷰모델
- 도메인에 걸쳐 있어 한 플러그인에 넣을 수 없는 조립물(예: 외형+대화의 NPC, 상호작용/아이템사용 어빌리티)

**경계 (비담당)**
- 전투·GAS 속성·락온·히트스톱 — [[WxCombat]]
- 인벤토리 데이터·보상 테이블 — [[WxInventory]]
- AI 인지·행동 컴포넌트·BT — [[WxAI]]
- 대화 세션·대화 액터 계약 — [[WxDialogue]]
- 퀘스트 진행 — [[WxQuest]]
- 뷰모델이 채우는 위젯·HUD 레이아웃·CommonUI 액션 — [[WxUI]]
- 상호작용 스캐너·스포너·소환 계약 — [[WxWorld]]
- 팀·스포너블 등 공용 정의 — [[WxCore]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 조립 최상단. 맵/프론트엔드 선택에 따라 폰 클래스를 고른다 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxPlayerController` | 폰 리스폰을 넘어 사는 플레이어 단위 상태(인벤토리·스캐너·대화 세션·레이아웃) 소유 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxCharacterBase` | 플레이어·적 공통 베이스. ASC와 전투 컴포넌트 합성의 중심 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력 소유 + 카메라·아이템사용·입력버퍼 파생 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 적 파생. AI 조립·상호작용·보상·소환·보스 표시를 엮는 지점 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxAIController` | AI 폰 전체의 컨트롤러. 인지·타겟을 락온으로 연결 | `Source/WxGame/Controller/WxAIController.h` |
| `UWxGameFlowSubsystem` | 프론트엔드 선택(폰·목적지)을 들고 맵 전환을 주도 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |

## 확장 포인트 / 규약
- 맵별 구성은 `AWxGameMode` 의 BP(GM_FrontEnd·GM_Combat)와 맵 WorldSettings 의 GameModeOverride 로 고른다. 폰은 프론트엔드 선택을 우선하고, 없으면 DefaultPawnClass 를 쓴다.
- 캐릭터 파생은 `AWxCharacterBase` 를 상속하고 도메인 컴포넌트를 생성자에서 붙이는 방식. 무기·부착물은 BP 디폴트의 ChildActorClass·에셋 지정으로 주입한다.
- 게임플레이 입력은 `UWxInputConfig` DataAsset 으로 주입한다. 어빌리티 발동 IA는 어빌리티 CDO가, 메뉴/상호작용 입력은 WxUI(CommonUI 액션·HUD 위젯)가 각각 소유하므로 여기 두지 않는다.
- 캐릭터 표시 데이터는 기존 `IWxUIData`로 제공한다. 플레이어 Resolver는 WxUI에서 Pawn의 ASC를 찾고, Character ViewModel이 인터페이스로 이름·초상화를 읽는다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 무엇이 무엇을 조립하는지, 폰 선택 규칙의 출발점
2. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어느 도메인 컴포넌트를 어떻게 합성하는지
3. `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` — 프론트엔드에서 게임플레이로 넘어가는 흐름
4. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModelResolver_PlayerCharacter.h` — Pawn의 ASC와 표시 데이터 소스를 연결하는 방식

## 관련
- 조립 대상: [[WxCombat]] · [[WxInventory]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxUI]] · [[WxWorld]] · [[WxCore]]

---
*문서 기준 커밋 `81c04f5` · 생성일 2026-09-11 · 소스 63파일 — `/readme-writer`로 갱신*
