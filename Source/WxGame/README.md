# WxGame — 게임 조립 모듈

> 도메인 플러그인들을 하나의 플레이 가능한 게임으로 엮는 기본 게임 모듈. 프레임워크 클래스(GameMode·Character·Controller 등)를 구현하고, 프론트엔드→게임 흐름, 그리고 WxUI 위젯을 도메인 데이터에 잇는 MVVM 브릿지를 담당한다.

## 책임
**담당**
- 프레임워크 클래스 구현·조립: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`, `AWxCharacterBase` 및 파생(`AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxNpc`), `AWxAIController`.
- 게임 조립: GameMode BP(GM_FrontEnd·GM_Combat)가 폰·컨트롤러·시작 아이템을 고르고, 컨트롤러·GameState 생성자가 도메인 컴포넌트를 기본 서브오브젝트로 붙인다.
- 프론트엔드↔게임 게임플로우: 메뉴에서 폰·레벨을 골라 트래블하고 도착을 검증하는 흐름.
- MVVM 브릿지: WxUI 위젯과 도메인 데이터 양쪽에 의존하는 뷰모델·리졸버(이 배선은 어느 한쪽 플러그인에도 둘 수 없다).
- 플레이어 입력 조립, 소비 아이템 사용, 치트.

**경계 (비담당)**
- 전투(ASC/GE/무기/락온/히트스톱) → [[WxCombat]]. 캐릭터는 컴포넌트를 소유만 한다.
- 인벤토리·장비·아이템 정의 → [[WxInventory]].
- UI 위젯·뷰모델 베이스 클래스 → [[WxUI]]. 여기 뷰모델은 그 베이스를 상속한다.
- 월드 오브젝트·상호작용 → [[WxWorld]] / AI 지각·StateTree → [[WxAI]] / 대화 → [[WxDialogue]] / 퀘스트 → [[WxQuest]].
- 공용 정의·태그·유틸 → [[WxCore]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 폰 클래스(FrontEnd 선택 → DefaultPawnClass)와 시작 아이템 지급(서버 전용). 맵별 구성은 BP 와 GameModeOverride | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | 퀘스트 컴포넌트를 기본 서브오브젝트로 소유 | `Source/WxGame/Framework/WxGameState.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스. ASC 를 직접 소유 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerController` | 인벤토리·스캐너·대화 세션·HUD 컴포넌트를 기본 서브오브젝트로 소유 | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxGameFlowSubsystem` | 프론트엔드→게임 트래블 상태머신(준비·이동·도착 검증) | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |
| `UWxViewModelResolver_PlayerCharacter` | 위젯에 폰/ASC 데이터를 주입하는 MVVM 리졸버(브릿지의 대표) | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- 새 게임플레이 구성은 `AWxGameMode` 의 BP 로 만들고(폰·컨트롤러·시작 아이템) 맵 WorldSettings 의 GameModeOverride 로 고른다. 같은 맵 다른 구성 실험은 엔진 URL 옵션 `?game=` 으로 한다.
- 새 캐릭터·컨트롤러는 대상 프레임워크 클래스를 상속한다. 플레이어 단위 도메인 컴포넌트는 `AWxPlayerController` 생성자에, 판 단위는 `AWxGameState` 생성자에 기본 서브오브젝트로 둔다. 띄울 HUD 는 컨트롤러 BP 의 HUD 컴포넌트 프로퍼티로 지정한다.
- 리플리케이션/권한: GameMode 는 서버에만 존재한다. 기본 서브오브젝트 컴포넌트는 양쪽에 있으므로 사이드 제한은 컴포넌트 자신의 가드(authority·로컬 컨트롤러)가 한다. 캐릭터 ASC 는 캐릭터가 소유(리스폰마다 스탯 재초기화라 PlayerState 불필요).
- 새 UI 데이터 배선은 `MVVM/` 에 뷰모델+리졸버 쌍을 둔다 — WxUI 위젯과 도메인 데이터 양쪽에 의존하므로 이 모듈에만 놓을 수 있다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 게임이 어떻게 시작되는지(폰 선택·시작 아이템)의 진입점.
2. `Source/WxGame/Controller/WxPlayerController.h` — 플레이어에 어떤 도메인 컴포넌트가 붙는지의 지도.
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트를 소유하고 무엇을 각 플러그인에 위임하는지의 지도.
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — 게임 데이터와 WxUI 위젯이 왜/어떻게 이 모듈에서 만나는지.

## 관련
- 상위: 이 모듈이 조립하는 도메인 플러그인 — [[WxCombat]] [[WxInventory]] [[WxUI]] [[WxWorld]] [[WxAI]] [[WxDialogue]] [[WxQuest]], 그리고 foundation [[WxCore]].

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 80파일 — `/readme-writer`로 갱신*
