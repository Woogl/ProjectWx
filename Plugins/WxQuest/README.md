# WxQuest — 퀘스트 시스템

> 게임의 퀘스트(임무) 진행을 담당하는 도메인 플러그인. StateTree 기반 구현을 전제로 한 골격 단계이며, 현재는 모듈 부트스트랩 코드만 존재한다.

## 책임
**담당**
- 퀘스트 시스템 런타임 모듈의 진입점 제공 (`WxQuest` 모듈 등록/로깅 카테고리)
- StateTree 기반 퀘스트 진행 로직의 향후 구현 컨테이너

**경계 (비담당)**
- 공용 정의(Gameplay Tag, Enum 등)는 [[WxCore]]에 위임
- 전투/인벤토리/UI 등 퀘스트 목표가 참조하는 시스템은 각 도메인 모듈([[WxCombat]], [[WxInventory]], [[WxUI]])에 위임

## 의존성
- **주요 의존**: [[WxCore]], `StateTree`(StateTreeModule), `GameplayTags`, `DeveloperSettings`
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxQuestModule` | 모듈 라이프사이클(StartupModule/ShutdownModule) | `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` |
| `LogWxQuest` | 모듈 전용 로그 카테고리 | `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` |

## 확장 포인트 / 규약
- 현재 퀘스트 도메인 타입(퀘스트/목표 클래스, DataTable Row, StateTree 스키마 등)은 아직 정의되지 않았다. 신규 구현은 `Public/`에 헤더를 추가하고 `Private/`에 구현을 두는 표준 모듈 구조를 따른다.
- 진행 로직은 `.uplugin`·`Build.cs`에 명시된 `StateTree`(StateTreeModule)를 기반으로 설계할 것.
- 공용 식별자(Gameplay Tag, Enum)는 [[WxCore]]에 선언하고 본 모듈에서 참조한다 (CLAUDE.md 규칙).

## 여기서부터 읽어라
1. `Plugins/WxQuest/WxQuest.uplugin` — 모듈 의도(StateTree 기반 퀘스트 시스템)와 플러그인 의존성 확인
2. `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs` — 빌드 의존 모듈 확인
3. `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h` — 모듈 진입점

## 관련
- 상위: 구체 컨텐츠를 조립하는 [[WxGame]] 게임 모듈에서 사용. 공용 정의는 [[WxCore]] 참조.

---
*문서 기준 커밋 `2983a08e` · 생성일 2026-06-11 · 소스 2파일 — `/readme-writer`로 갱신*
