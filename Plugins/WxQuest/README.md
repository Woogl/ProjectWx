# WxQuest — 퀘스트 시스템

> StateTree 기반 퀘스트 시스템 플러그인. 현재는 모듈 부트스트랩 골격만 존재하며, 퀘스트 진행/목표 추적 타입은 아직 구현되지 않았다.

## 책임
**담당**
- 퀘스트 시스템 런타임 모듈의 등록/생명주기 (`FWxQuestModule`)
- StateTree 기반 퀘스트 진행 로직의 향후 구현 컨테이너 및 `LogWxQuest` 로그 카테고리 정의

**경계 (비담당)**
- 공용 정의(Gameplay Tag, Enum 등)는 [[WxCore]]에 위임
- 전투/인벤토리/UI 등 퀘스트 목표가 참조하는 시스템은 각 도메인 모듈([[WxCombat]], [[WxInventory]], [[WxUI]])에 위임

## 의존성
- **주요 의존**: [[WxCore]], `StateTreeModule` (StateTree 플러그인 — 퀘스트 흐름 정의), `GameplayTags`, `DeveloperSettings`(Private)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxQuestModule` | 모듈 진입점 (현재 Startup/Shutdown 빈 구현) | `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` |
| `LogWxQuest` | 모듈 전용 로그 카테고리 | `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` |

## 확장 포인트 / 규약
- 현재 퀘스트 도메인 타입(퀘스트/목표 클래스, DataTable Row, StateTree 스키마 등)은 아직 정의되지 않았다. 신규 구현은 `Public/`에 헤더를 추가하고 `Private/`에 구현을 두는 표준 모듈 구조를 따른다.
- 퀘스트 흐름은 `.uplugin`·`Build.cs`에 명시된 `StateTree`(StateTreeModule) 기반으로 설계할 것. 새 목표/조건은 StateTree 노드(Task/Condition/Evaluator)로 추가한다.
- 에디터/데이터 주도 설정은 Private 의존 `DeveloperSettings`(`UDeveloperSettings` 파생)로 구성 예정.
- 최대 4인 멀티 환경이므로 퀘스트 상태/진행도는 권한(서버) 주도 + 리플리케이션 모델을 따를 것 (현재 미구현).

## 여기서부터 읽어라
1. `Plugins/WxQuest/WxQuest.uplugin` — 모듈 의도(StateTree 기반 퀘스트 시스템)와 플러그인 의존성(StateTree, WxCore) 확인
2. `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs` — 빌드 의존 모듈(StateTreeModule, GameplayTags, DeveloperSettings) 확인
3. `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` — 모듈 진입점, 이후 타입 추가의 시작점

## 관련
- 상위: 구체 컨텐츠를 조립하는 [[WxGame]] 게임 모듈에서 사용. 공용 정의는 [[WxCore]] 참조. 보상/목표 연동 후보는 [[WxInventory]], [[WxCombat]]

---
*문서 기준 커밋 `7a5764b` · 생성일 2026-06-12 · 소스 2파일 — `/readme-writer`로 갱신*
