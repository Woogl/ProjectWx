# WxQuest — 퀘스트 시스템

> StateTree 에셋 하나를 퀘스트 하나로 실행하고, 저널(제목·목표)을 서버 권위로 관리하는 도메인 플러그인. 무엇을 실행할지는 전부 데이터(에셋·태스크)가 지정하며, 코드는 어떤 퀘스트 에셋도 알지 않는다.

## 책임
**담당**
- 퀘스트 러너 소유·실행: GameState 에 부착된 `UWxQuestComponent` 가 권위 측에서만 순정 `UStateTreeComponent` 를 런타임 생성해 퀘스트 StateTree 를 구동한다.
- 저널 상태: 제목 1개 + 목표 N개(발급 핸들 기반)를 보관하고, 변경 시 `OnJournalChanged` 로 통지한다.
- 퀘스트 수주·체인·교체: 활성 퀘스트는 동시 1개이며, 새 시작은 기존을 정지하고 교체한다.
- 퀘스트 저작용 StateTree 태스크(제목/목표 설정, 도달 대기, 다음 퀘스트 시작) 제공.

**경계 (비담당)**
- 저널의 화면 표현·HUD 위젯 — [[WxUI]] 뷰모델이 `OnJournalChanged` 를 구독해 값을 pull.
- 부착 시점 결정 — GameMode 가 고른 Experience 에셋의 컴포넌트 주입 목록이 담당(GameState 는 본 클래스를 모른다).
- 보상 지급·월드 부수효과 — 퀘스트 StateTree 안의 크로스모듈 노드(예: GiveRewards)가 담당.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존), 엔진 서브시스템 `StateTree`/`GameplayStateTree`(StateTreeModule·GameplayStateTreeModule), `ModularGameplay`(`UGameStateComponent`), `UniversalObjectLocator`(태스크의 배치 액터 참조), `GameplayTags`.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (uplugin·Build.cs 모두 WxCore 만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착 러너·저널 관리자, 태스크의 위임 대상 | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 레벨 트리거 등에서 부르는 수주 진입점(`StartQuest`) | `Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `UWxQuestStateTree` | 퀘스트 1개 = 이 `UStateTree` 파생 에셋 1개 | `Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널에 제목 등록(목표 비움) | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 진입 시 목표 하나 걸고 이탈 시 걷어감(상태 수명=목표 수명) | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 0번 폰이 로케이터 대상 반경 진입까지 Running | `Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱에 예약(체인) | `Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- **새 퀘스트**: `UWxQuestStateTree` 에셋을 저작한다. 코드 수정 없이 데이터만으로 성립한다. 수주는 레벨 트리거 볼륨이 `UWxQuestLibrary::StartQuest` 에 에셋을 넘겨서, 체인은 `StartNextQuest` 태스크의 `Quest` 소프트 참조로 한다.
- **새 태스크**: `FStateTreeTaskCommonBase` 를 상속하고 `FInstanceDataType`·`GetInstanceDataType()`·`EnterState`(필요 시 `Tick`/`ExitState`) 를 구현한다. 컨텍스트 오너(GameState)에서 `UWxQuestComponent` 를 찾아 저널 갱신·체인을 위임하는 것이 기존 4개 태스크의 공통 패턴이다.
- **재진입 규약**: 러너 실행 콜스택 안에서는 에셋 교체가 엔진 가드에 막힌다. 콜스택 밖이면 `ActivateQuest`, 태스크 내부 등 콜스택 안이면 다음 틱으로 미루는 `RequestActivateQuest` 를 쓴다.
- **권위 규약**: 러너는 권위(서버/리슨 호스트)에만 존재한다. 라이브러리·태스크는 비-권위에서 조용히 무시된다. v1 은 싱글/리슨 호스트 전제(0번 컨트롤러 사용).
- **저널 정리**: 태스크가 아니라 러너의 실행 상태 변경 통지(`HandleStateTreeRunStatusChanged`)로 한다 — 완료·실패·교체 세 종료 경로가 한 곳으로 수렴한다.
- **완료 판정 함정**: `SetQuestTitle`/`SetQuestObjective`/`StartNextQuest` 는 진입 즉시 Succeeded 를 내고 상태 완료는 짝이 되는 `WaitMoveToTarget` 등 대기 태스크가 낸다. 한 상태에서 완료 판정(`bConsideredForCompletion`) 태스크를 전부 빼면 그 상태는 형제 상태의 완료를 물려받는다 — 제목/목표 태스크의 판정 참여 여부가 트리 진행에 영향을 주므로 주의한다.

## 여기서부터 읽어라
1. `Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 doc-comment 에 러너 소유·권위·데이터 불가지·저널 정리 수렴이 전부 정리돼 있다. 모듈 전체의 설계 요지.
2. `Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 생성·상태 통지·다음 틱 활성화의 실제 흐름.
3. `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` — 태스크가 저널에 어떻게 위임하는지의 대표 사례(상태 수명=목표 수명).

## 관련
- 상위: 저널을 구독하는 [[WxUI]] HUD 뷰모델. 부착을 지시하는 Experience 에셋(GameMode 경유). 보상 등 크로스모듈 노드가 얹히는 퀘스트 StateTree.

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 15파일 — `/readme-writer`로 갱신*
