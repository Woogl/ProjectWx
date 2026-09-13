# WxGame — 게임 조립 모듈

> 도메인 플러그인(Combat·Inventory·AI·Dialogue·Quest·World·UI)을 하나의 플레이 가능한 게임으로 조립하는 최상위 게임 소스 모듈. 프레임워크 클래스(GameMode·Controller·Character)와 프론트엔드→맵→폰 흐름을 소유한다.

## 책임
**담당**
- 프레임워크 배선: GameMode/Controller/Character/PlayerState/GameState 를 구현하고 도메인 컴포넌트를 이들에 부착한다.
- 프론트엔드 선택 → 맵 전환 → 도착 GameMode 의 폰 결정으로 이어지는 게임 진입 흐름.
- 플레이어 입력(이동/시선/점프/크라우치)과 플레이어 캐릭터의 카메라·GAS·전투/인벤 컴포넌트 조립.
- 여러 도메인이 만나는 지점의 합성 액터(외형+대화 NPC, AI+상호작용+보상 적 캐릭터)와 MVVM 리졸버/뷰모델(도메인 데이터를 UI 뷰모델로 연결).

**경계 (비담당)**
- 전투 규칙·대미지·락온 로직은 [[WxCombat]], 인벤토리·보상 테이블은 [[WxInventory]], AI 판단·행동트리는 [[WxAI]], 대화 세션은 [[WxDialogue]], 퀘스트 진행은 [[WxQuest]], 상호작용/스폰/월드는 [[WxWorld]], 위젯·HUD 는 [[WxUI]] 에 위임한다. 이 모듈은 그 컴포넌트를 폰/컨트롤러에 붙여 배선만 한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 맵별 BP·WorldSettings 오버라이드로 모드를 고르고, 프론트엔드 선택 폰을 폰 클래스로 배선 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxGameFlowSubsystem` | GameInstance 차원에서 선택 폰·목적지를 들고 맵을 열어 도착 GameMode 로 넘기는 전환 허브 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스, ASC·AttributeSet·전투 컴포넌트를 캐릭터에 직접 소유 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력과 카메라·아이템 사용·입력 버퍼를 소유하는 플레이어 폰 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | AI 행동·상호작용·보상·보스 표시를 합성한 적 폰 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 폰 리스폰에도 살아남는 플레이어 단위 컴포넌트(인벤·스캐너·대화·레이아웃) 소유 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxAIController` | AI 가 모는 폰 전부의 컨트롤러 — 감각 설정과 BT 타겟→락온 배선 | `Source/WxGame/Controller/WxAIController.h` |
| `UWxViewModelResolver_Ability` | 위젯 소유 폰의 ASC 에서 슬롯 뷰모델을 얻는 MVVM 리졸버(리졸버·뷰모델군의 대표 진입) | `Source/WxGame/MVVM/WxViewModelResolver_Ability.h` |

## 확장 포인트 / 규약
- 맵별 구성은 `AWxGameMode` 의 BP(GM_FrontEnd·GM_Combat)와 맵 WorldSettings 의 GameModeOverride 로 고른다. 폰은 프론트엔드 선택이 있으면 그것을, 없으면 DefaultPawnClass 를 쓴다.
- 플레이어 단위 상태는 `AWxPlayerController` 가 소유한다(폰 리스폰에도 유지). 캐릭터는 ASC 를 직접 소유하고 리스폰마다 스탯을 재초기화하므로 `AWxPlayerState` 는 비어 있다.
- 입력은 `UWxInputConfig` 데이터에셋 주도이며, 어빌리티 입력은 AbilitySet 부여 대상에서 파생하고 메뉴 입력은 CommonUI 액션으로 처리한다.
- 치트는 `UWxCheatManager` 가 `AWxPlayerController` 의 CheatClass 로 배선되며 Standalone·에디터에서만 생성된다.

## 여기서부터 읽어라
1. `Source/WxGame/WxGame.Build.cs` — 이 모듈이 모든 Wx 도메인 플러그인을 모으는 조립 지점임을 의존 목록에서 확인.
2. `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` + `Source/WxGame/Framework/WxGameMode.h` — 프론트엔드 선택→맵 전환→폰 결정 흐름.
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트를 어디에 붙이는지 배선의 중심.

## 관련
- 상위: 게임 실행 진입(모듈 등록은 `Source/WxGame/WxGame.h`). 여기서 조립하는 도메인 플러그인 [[WxCombat]] · [[WxInventory]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxWorld]] · [[WxUI]] · [[WxCore]] 와 함께 본다.

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 58파일 — `/readme-writer`로 갱신*
