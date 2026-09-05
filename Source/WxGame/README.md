# WxGame — 게임 조립 모듈

> 도메인 플러그인들을 하나의 플레이 가능한 게임으로 엮는 기본 게임 모듈. 프레임워크 클래스(GameMode·Character·Controller 등)를 구현하고, Experience/GameFeature 부트스트랩과 프론트엔드→게임 흐름, 그리고 WxUI 위젯을 도메인 데이터에 잇는 MVVM 브릿지를 담당한다.

## 책임
**담당**
- 프레임워크 클래스 구현·조립: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`, `AWxCharacterBase` 및 파생(`AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxNpc`), `AWxAIController`.
- Experience/GameFeature 부트스트랩 (Lyra 이식): Experience 확정→복제→비동기 로드→액션 실행 파이프라인.
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
| `AWxGameMode` | 판의 Experience 를 확정해 매니저에 넘김(서버 전용). 게임 부트스트랩의 시작점 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성 데이터 에셋(GameFeature·액션·기본 폰) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | GameState 에 붙어 Experience 를 복제·로드·적용하는 주체 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스. ASC 를 직접 소유, ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerController` | Experience 가 요청한 컨트롤러 컴포넌트를 주입받는 receiver | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxGameFlowSubsystem` | 프론트엔드→게임 트래블 상태머신(준비·이동·도착 검증) | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 GameFeature 액션 | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `UWxViewModelResolver_PlayerCharacter` | 위젯에 폰/ASC 데이터를 주입하는 MVVM 리졸버(브릿지의 대표) | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- 새 게임플레이 구성은 `UWxExperienceDefinition` 에셋으로 만들고, GameFeature 플러그인 이름은 `GameFeaturesToEnable`, 배선은 `Actions`/`ActionSets` 로 데이터 주도 지정한다. 컴포넌트 주입은 `UWxGameFeatureAction_AddComponents` 를 쓴다(스톡 AddComponents 대체).
- Experience 확정 순서: 진입 URL `?Experience=이름` → `AWxWorldSettings::GetDefaultGameplayExperience`. 둘 다 비면 무효 ID → 매니저가 에러로 드러낸다. 폴백 없음.
- 새 캐릭터·컨트롤러는 대상 프레임워크 클래스를 상속하고 컴포넌트는 Experience 액션으로 주입한다(하드코딩 대신). 각 클래스는 ModularGameplay receiver 로 opt-in 돼 있다.
- 리플리케이션/권한: GameMode 는 서버에만 존재하고, Experience 참조 복제로 클라 적용이 성립한다(서버 직접 호출 / 클라 OnRep 이 같은 파이프라인 주행). 캐릭터 ASC 는 캐릭터가 소유(리스폰마다 스탯 재초기화라 PlayerState 불필요).
- 새 UI 데이터 배선은 `MVVM/` 에 뷰모델+리졸버 쌍을 둔다 — WxUI 위젯과 도메인 데이터 양쪽에 의존하므로 이 모듈에만 놓을 수 있다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 게임이 어떻게 시작되는지(Experience 확정→폰 스폰 지연)의 진입점.
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — Experience 로드 파이프라인의 상태 전이 전체가 함수 주석에 정리돼 있다.
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트를 소유하고 무엇을 각 플러그인에 위임하는지의 지도.
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — 게임 데이터와 WxUI 위젯이 왜/어떻게 이 모듈에서 만나는지.

## 관련
- 상위: 이 모듈이 조립하는 도메인 플러그인 — [[WxCombat]] [[WxInventory]] [[WxUI]] [[WxWorld]] [[WxAI]] [[WxDialogue]] [[WxQuest]], 그리고 foundation [[WxCore]]. GameFeature 콘텐츠 플러그인(`Plugins/GameFeatures/`)이 Experience 를 통해 이 모듈 위에 얹힌다.

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 80파일 — `/readme-writer`로 갱신*
