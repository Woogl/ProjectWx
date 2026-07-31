# WxQuest — 퀘스트 시스템

> 퀘스트 1개를 StateTree 에셋 1개로 정의하고, GameState에 붙은 컴포넌트가 서버 권위로 실행·저널(제목·목표)을 관리한다. 활성 퀘스트는 동시 1개이며, 수주·체인·임의 시작 모두 데이터가 지정한다.

## 책임
**담당**
- 퀘스트 StateTree 러너 실행 (`UWxQuestComponent`, 권위 측만 구동)
- 저널 관리: 제목 등록, 목표 add/remove(핸들 기반), 종료(완료·실패·교체) 시 자동 정리
- 퀘스트 노드 제공: SetQuestTitle / SetQuestObjective / WaitMoveToTarget / StartNextQuest
- 수주 경로: `bAutoStart` 에셋 자동 탑재, 체인(StartNextQuest), 임의 시작(`UWxQuestLibrary`)

**경계 (비담당)**
- HUD 표시 — 저널 변경 델리게이트만 방출, 뷰모델 구독은 [[WxUI]]
- 컴포넌트 부착 — GameMode가 고른 Experience 에셋의 주입 목록이 담당(코드 부착 아님)
- 에셋 신규 생성 팩토리 — WxEditor
- 보상 지급 등 크로스모듈 부수효과 — 별도 ST 노드/도메인

## 의존성
- **주요 의존**: `WxCore`(`FWxActorTarget` 레벨 액터 지정), StateTree / GameplayStateTree / ModularGameplay / UniversalObjectLocator, AssetRegistry(수주 발견), DeveloperSettings
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 러너 구동 + 저널 관리의 중심 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestStateTree` | 퀘스트 1개 = ST 에셋 1개. `bAutoStart` 수주 플래그 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널 제목 등록 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_SetQuestObjective` | 진입 시 목표 걸고 이탈 시 걷어감(수명=상태 수명) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰의 대상 반경 도달 대기 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트 시작을 다음 틱 예약(체인) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `UWxQuestLibrary` | BP 진입점. 레벨 스크립트→컴포넌트 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |

## 확장 포인트 / 규약
- **새 퀘스트 노드**: `FStateTreeTaskCommonBase` 상속, InstanceData 구조체 + `FInstanceDataType` 별칭. 오너(GameState)에서 `UWxQuestComponent`를 찾아 위임하고, 컴포넌트 부재 시 Failed(러너 밖 오조립) + `LogWxQuest` 경고. 단 `bConsideredForCompletion=false`인 저널 태스크(SetQuestTitle·SetQuestObjective)는 엔진이 반환 상태를 무시하므로 실제 진단은 그 경고 로그가 전부다.
- **저널 태스크는 상태 완료 판정에서 빠진다** — 판정에 끼면 자식 있는 상태가 즉시 완료돼 퀘스트가 관통된다. 상태 완료는 짝이 되는 Wait 태스크가 낸다.
- **저널 정리는 노드가 아님** — 트리 종료를 컴포넌트가 감지해 자동 정리(완료·실패·교체 세 경로 수렴).
- **러너 재시작은 다음 틱** — ST 실행 콜스택 안 재진입 금지. 임의 시작은 `RequestStartQuest`.
- 레벨 액터 지정은 `FWxActorTarget`(FUniversalObjectLocator)로 매 틱 SyncFind — 기믹 노드의 권위/스킵 게이트를 복사하지 말 것(러너가 이미 권위 전용).

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 시스템 전체 설계(러너 위임·저널·수주 경로)가 헤더 주석에 응축돼 있다
2. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` — 네 노드의 계약과 저널 태스크의 완료 판정 함정
3. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 권위 게이트, 다음 틱 시작, RunStatus 콜백 정리 로직의 실제

## 관련
- 상위: [[WxCore]] (FWxActorTarget)
- 소비: [[WxUI]] (저널 델리게이트 구독)

---
*문서 기준 커밋 `59acb24` · 생성일 2026-07-30 · 소스 9파일 — `/readme-writer`로 갱신*
