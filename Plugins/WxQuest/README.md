# WxQuest — 퀘스트 시스템

> StateTree 기반 퀘스트 시스템 플러그인. 현재는 모듈 부트스트랩 골격만 존재하며, 퀘스트 진행/목표 추적 타입은 아직 구현되지 않았다.

## 책임
**담당**
- 퀘스트 시스템 런타임 모듈의 등록/생명주기 (`FWxQuestModule`)
- StateTree 기반 퀘스트 진행 로직의 향후 구현 컨테이너 및 `LogWxQuest` 로그 카테고리 정의

**경계 (비담당)**
- 공용 정의(Gameplay Tag, Enum 등)는 [[WxCore]]에 위임
- 퀘스트 목표가 참조하는 외부 시스템(전투/인벤토리/UI 등)은 각 도메인 모듈에 위임 — *(연동 코드는 아직 없음)*

## 의존성
- **주요 의존**: [[WxCore]], `StateTreeModule` (StateTree 플러그인 — 퀘스트 흐름 정의), `GameplayTags`, `DeveloperSettings`(Private)
- 규칙: 플러그인 의존이 `WxCore` + 엔진 플러그인(`StateTree`)뿐 — WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxQuestModule` | 모듈 진입점 (현재 Startup/Shutdown 빈 구현) | `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` |
| `LogWxQuest` | 모듈 전용 로그 카테고리 | `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` |

> 퀘스트 도메인 타입(퀘스트/목표 클래스, DataTable Row, StateTree 스키마 등)은 아직 없음. 신규 코드 추가 시 이 표를 갱신할 것.

## 확장 포인트 / 규약
- 현재 퀘스트 도메인 타입은 미정의 상태다. 신규 구현은 `Public/`에 헤더를, `Private/`에 구현을 두는 표준 모듈 구조를 따른다.
- 퀘스트 흐름은 `.uplugin`·`Build.cs`에 명시된 `StateTree`(StateTreeModule) 기반으로 설계할 것. 새 목표/조건은 StateTree 노드(Task/Condition/Evaluator)로 추가한다.
- 데이터 주도 설정은 Private 의존 `DeveloperSettings`(`UDeveloperSettings` 파생)로 구성 예정.
- 공용 타입이 필요하면 [[WxCore]]에 두고 참조한다 (플러그인 간 직접 참조 금지 규칙).

## 여기서부터 읽어라
1. `Plugins/WxQuest/WxQuest.uplugin` — 모듈 의도(StateTree 기반 퀘스트 시스템)와 플러그인 의존성(StateTree, WxCore) 확인
2. `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs` — 빌드 의존 모듈(StateTreeModule, GameplayTags, DeveloperSettings) 확인
3. `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` — 모듈 진입점, 이후 타입 추가의 시작점

## 관련
- 기반: 공용 정의는 [[WxCore]] 참조
- 상위: 구체 컨텐츠를 조립하는 [[WxGame]] 게임 모듈에서 사용

---
*문서 기준 커밋 `a2ba2b5` · 생성일 2026-06-17 · 소스 2파일 — `/readme-writer`로 갱신*
