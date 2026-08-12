# WxQuest — 퀘스트 시스템

> 퀘스트 1개를 StateTree 에셋 1개로 표현하고, GameState 에 부착된 컴포넌트가 서버 권위로 실행·저널(제목·목표)을 관리하는 데이터 주도 퀘스트 플러그인.

## 책임
**담당**
- 퀘스트 StateTree 러너 소유·실행: `UWxQuestComponent` 가 권위 측에서 순정 `UStateTreeComponent` 를 런타임 생성해 실행을 위임. 활성 퀘스트는 동시 1개(새 시작은 교체).
- 저널 상태 관리: 제목 등록·목표 추가/제거를 발급 핸들로 관리하고, 러너 종료(완료·실패·교체)를 감지해 자동 정리. 변경 시 `OnJournalChanged` 발화.
- 퀘스트 진행 노드 제공: `SetQuestTitle` / `SetQuestObjective` / `WaitMoveToTarget` / `ActivateNextQuest` StateTree 태스크.
- 외부 진입점: `UWxQuestLibrary` 가 월드 GameState 의 컴포넌트를 찾아 활성화·이벤트 전송을 위임(트리거 볼륨 등 레벨 배치 액터에서 호출).
- 퀘스트 전용 에셋 타입 `UWxQuestStateTree` — 지정 필드·컴포넌트 API 가 이 타입만 받아 일반 ST 오지정을 픽커·컴파일 단계에서 차단.

**경계 (비담당)**
- 퀘스트 에셋 신규 생성 팩토리(WxEditor 담당) — 본 런타임 모듈은 에셋 불가지.
- HUD 표시: 저널 값은 뷰모델이 `OnJournalChanged` 구독 후 pull 하며, 위젯은 UI 모듈 소관.
- 컴포넌트 부착: GameMode 가 고른 Experience 에셋의 주입 목록이 담당(GameState 는 본 클래스를 모름).

## 의존성
- **주요 의존**: `WxCore`. 엔진 서브시스템으로 StateTree(`StateTreeModule`) / GameplayStateTree(`GameplayStateTreeModule`) / `ModularGameplay`(GameState 컴포넌트) / `UniversalObjectLocator`(레벨 액터 지정).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착, 권위 측 러너 소유·실행·저널 관리 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 레벨/BP 진입점 — 컴포넌트를 찾아 활성화·이벤트 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `UWxQuestStateTree` | 퀘스트 1개를 담는 전용 StateTree 에셋 타입 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `FWxStateTreeTask_SetQuestTitle` | 저널을 제목으로 등록(진행 시작 상태에 1회) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 진입 시 목표 등록·이탈 시 제거(상태 수명 = 표시 수명) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 대상 반경 도달까지 대기, 도달 시 완료 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_ActivateNextQuest` | 다음 퀘스트 시작을 예약하고 즉시 완료(체인) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_ActivateNextQuest.h` |

## 확장 포인트 / 규약
- 새 퀘스트 태스크는 `FStateTreeTaskCommonBase` 를 상속해 태스크당 헤더/소스 한 쌍(`Public|Private/Quest/WxStateTreeTask_<이름>.{h,cpp}`)으로 추가. 인스턴스 데이터는 별도 `USTRUCT`(`FInstanceDataType`)로 두고 `GetInstanceDataType()` 을 오버라이드.
- 노드는 컨텍스트 오너(GameState)에서 `UWxQuestComponent` 를 찾아 저널·체인을 위임한다. 러너가 권위에만 존재하므로 권위 게이트가 없고, 세이브 미도입이라 초기 진입 스킵 게이트도 없다 — 첫 선택이 곧 정상 실행 경로다(기믹 노드의 두 게이트를 복사하지 말 것).
- 완료 판정 규약: 저널 태스크는 완료를 내지 않고 상태에 머물며(표시의 수명 = 그 상태의 수명), 완료는 짝이 되는 Wait 태스크가 낸다. 판정 참여 여부는 그 태스크가 어떤 상태에 얹히는지로 갈린다 — `SetQuestTitle` 은 자식을 둔 상태에 홀로 얹혀 참여하고, `SetQuestObjective` 는 대기 태스크와 같은 상태(TasksCompletion=All)에 얹히므로 빠진다(끼면 그 상태가 영영 완료되지 않는다).
- 어떤 상태든 완료 판정 태스크를 최소 하나 가져야 한다. 하나도 없으면 엔진이 "상태 자신" 비트를 덧붙이는데, 형제 상태끼리 완료 비트 범위를 공유하는 데다 진입 시 비트를 씻는 `ResetStatus` 가 태스크 개수만큼만 지워 이 덧붙은 비트를 남긴다. 그러면 형제가 앞서 세운 완료를 그대로 물려받아 진입하자마자 완료로 읽히고, 완료 전이 탐색이 가장 얕은 상태부터 도므로 자식의 전이는 검토조차 되지 않은 채 루트 재선택으로 떨어진다.
- 레벨 액터 지정은 `FUniversalObjectLocator` 를 배열로 받는다(엔진 5.8 의 직속 UOL 값 위젯 제한 회피). 해석은 매 틱 `SyncFind`(강제 로드 없음)로 캐시 없이 수행.
- 새 퀘스트 에셋은 스키마가 `StateTreeComponentSchema` 로 고정된 `UWxQuestStateTree` 로 만든다.

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 러너 소유·저널·권위 모델 등 시스템 전체 설계가 클래스 주석에 응축.
2. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_*.h` — 태스크당 파일 하나이고 각 헤더 주석이 그 노드의 계약을 담는다(완료 판정 규약은 위 「확장 포인트」 참조).
3. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 다음 틱 예약·러너 상태 변경 처리 등 실제 제어 흐름.

## 관련
- 상위: [[WxGame]] (Experience 주입), [[WxCore]] (foundation)

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 9파일 — `/readme-writer`로 갱신*
