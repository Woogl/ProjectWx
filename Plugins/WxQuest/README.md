# WxQuest — 퀘스트 시스템

> StateTree 에셋 하나를 퀘스트 하나로 실행하고, 제목·목표로 이루어진 저널을 서버 권위로 관리한다. 수주·진행·체인·완료의 전 종료 경로가 GameState 부착 컴포넌트 한 곳으로 수렴한다.

## 책임
**담당**
- 활성 퀘스트(동시 1개)의 StateTree 러너 구동 — 권위 머신에서만 러너를 띄워 월드 부수효과의 단일 구동을 보장
- 저널 상태(제목·목표 목록) 보관과 변경 통지(`OnJournalChanged`)
- 퀘스트 수주·교체·체인 요청의 진입점 제공 및 실행 콜스택 재진입 회피(다음 틱 예약)
- 퀘스트 저작용 StateTree 태스크 노드 4종 제공(제목/목표/다음 퀘스트/도달 대기)

**경계 (비담당)**
- 퀘스트 데이터 자체(무엇을 실행할지) — 전부 `UWxQuestStateTree` 에셋과 태스크 파라미터가 데이터로 지정, 컴포넌트는 에셋 불가지
- 컴포넌트 부착 — GameMode가 고른 Experience 에셋의 주입 목록이 담당(GameState는 본 클래스를 모름)
- 저널의 화면 표시 — HUD 뷰모델이 `OnJournalChanged`를 구독해 pull ([[WxUI]])
- 보상 지급 등 도메인 부수효과 — 크로스모듈 StateTree 노드에 위임

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착 러너·저널 관리자. 모든 제어 흐름의 수렴점 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 레벨 트리거 볼륨 등에서 수주하는 BP 진입점(`StartQuest`) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `UWxQuestStateTree` | 퀘스트 1개를 나타내는 StateTree 에셋 타입(마커) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `FWxStateTreeTask_SetQuestTitle` | 저널 제목 등록 태스크(진입 시) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명과 목표 수명을 묶는 목표 태스크(진입 시 걸고 이탈 시 걷음) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_StartNextQuest` | 퀘스트 체인 — 다음 퀘스트 다음 틱 예약 후 즉시 완료 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰의 대상 반경 도달까지 Running 대기 후 완료 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |

## 확장 포인트 / 규약
- **새 퀘스트 만들기**: `UWxQuestStateTree` 에셋을 저작하고 태스크 노드를 배치한다. 코드 변경 불필요 — 컴포넌트는 어떤 에셋도 알지 않는다.
- **새 퀘스트 태스크 추가**: `FStateTreeTaskCommonBase`를 상속한 USTRUCT를 만들고, 컨텍스트 오너(GameState)에서 `FindComponentByClass<UWxQuestComponent>()`로 컴포넌트를 찾아 위임한다. 컴포넌트 부재는 "러너 밖 조립 오류"로 보고 경고 후 무시하는 것이 기존 태스크의 규약이다.
- **완료 판정 규약**: 제목·목표 설정 태스크는 완료 판정에서 빠져 진입 즉시 Succeeded여도 상태를 끝내지 않는다. 상태 완료는 짝이 되는 Wait 계열 태스크가 낸다.
- **재진입 규약**: 러너 실행 콜스택 안에서의 퀘스트 교체는 엔진이 거부하므로 `RequestActivateQuest`(다음 틱 예약)를 쓴다. 콜스택 밖에서만 `ActivateQuest` 직접 호출이 안전하다.
- **리플리케이션 모델**: 러너·저널은 권위(싱글/리슨 호스트)에만 존재한다. 저널 정리는 태스크가 아니라 러너 실행 상태 변경 통지(`HandleStateTreeRunStatusChanged`)로 하여 완료·실패·교체 세 경로를 한 곳으로 모은다.
- **대상 지정 규약**: `WaitMoveToTarget`은 `FUniversalObjectLocator`로 배치 액터를 지정한다(순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증을 우회, WP 언로드/재로드 자연 처리).

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 doc-comment가 러너 소유·권위·재진입·에셋 불가지 등 모듈의 설계 전제를 전부 담고 있다. 여기부터 잡는다.
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 런타임 생성, 다음 틱 예약, 저널 정리 콜백의 실제 흐름
3. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp` — 수주 진입점이 GameState 컴포넌트를 어떻게 찾아 위임하는지(짧다)

## 관련
- 상위: 레벨 배치 트리거 볼륨(수주), Experience 에셋(컴포넌트 주입), 저널을 구독하는 HUD ([[WxUI]])
- 함께: [[WxCore]] (공용 정의 의존)

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 15파일 — `/readme-writer`로 갱신*
