# WxGame — 게임 조립 모듈

> 프레임워크 진입점(GameMode·GameState·Controller·Character)을 두고, 전투·인벤토리·UI·대화·퀘스트·AI·월드 플러그인을 실제 폰과 컨트롤러 위에 조립·배선하는 기본 게임 모듈이다. 시스템 자체의 로직은 각 Wx 플러그인이 갖고, 여기서는 그것들을 이어 붙인다.

## 책임
**담당**
- 프레임워크 진입점: `AWxGameMode`(맵별 폰 선택)·`AWxGameState`·`AWxPlayerState`·`AWxPlayerController`·`AWxAIController`.
- 캐릭터 계층: 공통 베이스 `AWxCharacterBase`(ASC를 캐릭터가 직접 소유, 팀·죽음·래그돌 처리)와 파생 `AWxPlayerCharacter`·`AWxEnemyCharacter`·`AWxNpc`.
- 플레이어 단위 컴포넌트 배선: 인벤토리·상호작용 스캐너·대화 세션·레이아웃을 `AWxPlayerController`에, 카메라·아이템 사용·입력 버퍼를 `AWxPlayerCharacter`에 부착.
- 프론트엔드→게임 흐름: `UWxGameFlowSubsystem`이 고른 폰·맵을 들고 트래블, 도착한 GameMode가 그 폰을 씀.
- 게임 고유 어빌리티/노티파이 접착: `UWxAbility_Interact`·`UWxAbility_UseItem`·`UWxAnimNotify_UseItem`.
- UI 바인딩 접착(MVVM): 각 시스템 데이터를 위젯에 물리는 `WxViewModel_*`·`WxViewModelResolver_*`.
- 입력 구성(`UWxInputConfig`), 치트(`UWxCheatManager`).

**경계 (비담당)**
- 전투 로직(ASC·AttributeSet·락온·히트스톱·무기·피니셔) — [[WxCombat]]. 여기서는 컴포넌트를 캐릭터에 소유·복제만 함.
- 아이템·보상 데이터와 인벤토리 저장 — [[WxInventory]]. `UWxItemUseComponent`는 사용 요청을 넘길 뿐.
- HUD·레이아웃 위젯과 ViewModel 베이스/리졸버 프레임워크 — [[WxUI]].
- 대화 진행 — [[WxDialogue]], 퀘스트 상태 — [[WxQuest]], 감지·행동트리 — [[WxAI]], 스폰·상호작용·소환물 인터페이스 — [[WxWorld]].
- 공용 정의·기반 — [[WxCore]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 맵별(GM_FrontEnd·GM_Combat) 구성, 폰 클래스 선택 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | 판 전체 상태(QuestComponent 거주처) | `Source/WxGame/Framework/WxGameState.h` |
| `AWxPlayerController` | 플레이어 단위 컴포넌트 소유(인벤토리·스캐너·대화·레이아웃) | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxAIController` | AI 폰 전체의 컨트롤러, 인지·타겟팅 배선 | `Source/WxGame/Controller/WxAIController.h` |
| `AWxCharacterBase` | 공통 베이스, ASC 직접 소유·팀·죽음·래그돌 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력·카메라·아이템 사용 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 적 AI 조립·상호작용·보상·보스 표시 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `UWxGameFlowSubsystem` | 프론트엔드 선택 폰·맵으로 트래블 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |

## 확장 포인트 / 규약
- 맵 구성은 GameMode BP(GM_FrontEnd·GM_Combat)와 맵 WorldSettings의 GameModeOverride로 고른다. 폰은 프론트엔드 선택 우선, 없으면 DefaultPawnClass(프론트엔드는 SpectatorPawn).
- ASC는 PlayerState가 아니라 캐릭터가 소유한다 — 리스폰마다 스탯을 새로 초기화하므로 `AWxPlayerState`는 아직 비어 있다.
- 캐릭터의 무기·부착물·초상화·팀은 BP 디폴트(ChildActorClass, MetaHuman 에셋, `Portrait`, `Team`)로 데이터 주도 설정한다.
- 적 캐릭터의 보스 여부·소환물 카운트·보상은 `AWxEnemyCharacter`의 EditDefaults 프로퍼티(`bIsBoss`·`MaxCountPerMaster`·`RewardRow` DataTable)로 지정한다.
- 게임플레이 입력은 `UWxInputConfig` DataAsset으로 주입, 어빌리티 입력은 태그로 라우팅된다.

## 여기서부터 읽어라
1. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 컴포넌트를 소유하고 각 시스템이 어느 플러그인에서 오는지 한눈에 보이는 조립 지도.
2. `Source/WxGame/Controller/WxPlayerController.h` — 폰 리스폰에도 살아남는 플레이어 단위 상태(인벤토리·대화·레이아웃)의 소유처.
3. `Source/WxGame/Framework/WxGameMode.h` — 맵·폰 선택 규칙. 프론트엔드→전투 흐름의 시작점.
4. `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` — 프론트엔드 선택이 실제 맵 트래블로 이어지는 통로.

## 관련
- 조립 대상 시스템: [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxDialogue]] · [[WxQuest]] · [[WxAI]] · [[WxWorld]]
- 기반: [[WxCore]]

---
*문서 기준 커밋 `ffc6360` · 생성일 2026-09-10 · 소스 65파일 — `/readme-writer`로 갱신*
