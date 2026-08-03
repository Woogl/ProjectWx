# WxQuest — 퀘스트 시스템

> 퀘스트 1개를 StateTree 에셋 1개로 담아, GameState 에 서버 권위로 붙는 러너가 실행하고 저널(제목·목표)을 관리하는 시스템. 활성 퀘스트는 동시 1개(새 시작은 교체)이며, 수주·체인 모두 데이터가 지정한다.

## 책임
**담당**
- 퀘스트 StateTree 러너의 권위 측 구동과 실행 상태(완료·실패·교체) 수렴 처리 (`UWxQuestComponent`)
- 저널 관리: 제목 등록, 목표 add/remove(핸들 기반), 종료 시 자동 정리, `OnJournalChanged` 통지
- 퀘스트 전용 StateTree 노드 제공: SetQuestTitle / SetQuestObjective / WaitMoveToTarget / ActivateNextQuest
- 수주 2경로: 레벨 배치 볼륨 등 외부 시작(`UWxQuestLibrary`), 체인(ActivateNextQuest)

**경계 (비담당)**
- 저널의 HUD 표시 — 델리게이트만 방출, 뷰모델 구독은 [[WxUI]]
- 컴포넌트 부착 — GameMode 가 고른 Experience 에셋의 주입 목록이 담당(코드 부착 아님)
- 에셋 신규 생성 팩토리 — WxEditor(스키마 `StateTreeComponentSchema` 고정)
- 보상 지급 등 크로스모듈 부수효과 — 본 모듈 밖 ST 노드/도메인

## 의존성
- **주요 의존**: `WxCore`(`FWxActorTarget` 레벨 액터 지정), StateTree / GameplayStateTree(러너·노드 베이스), ModularGameplay(`UGameStateComponent`), UniversalObjectLocator, DeveloperSettings
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 러너 구동 + 저널 관리의 허브 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestStateTree` | 퀘스트 1개 = ST 에셋 1개. 오지정을 막는 타입 마커 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `UWxQuestLibrary` | BP 진입점(`ActivateQuest`/`SendQuestEvent`). 시작 볼륨·레벨 스크립트→컴포넌트 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널 제목 등록. 완료를 내지 않고 상태에 상주(완료 판정 참여) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 동안 목표 표시(진입 등록·이탈 회수). 완료 판정 미참여 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 대상 반경 도달까지 Running 대기, 도달 시 완료 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_ActivateNextQuest` | 다음 퀘스트 활성화를 다음 틱 예약, 즉시 완료(체인) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |

## 확장 포인트 / 규약
- **새 퀘스트**: `UWxQuestStateTree` 에셋 생성(WxEditor 팩토리). 탑재는 레벨 배치 시작 볼륨 등이 `UWxQuestLibrary::ActivateQuest` 로 지정하고, 그 뒤 진행 개시는 퀘스트 자신의 Start 게이트 태스크가 판정한다.
- **새 노드**: `FStateTreeTaskCommonBase` 상속 + InstanceData 구조체 + `FInstanceDataType` 별칭. 오너(GameState)에서 `UWxQuestComponent` 를 찾아 위임하고, 컴포넌트 부재는 오조립(러너 밖 사용)이다.
- **저널 태스크의 완료 판정 참여는 얹히는 상태로 갈린다** — SetQuestTitle 은 자식을 둔 상태에 홀로 얹혀 판정에 참여하고(빠지면 형제 완료를 물려받음), SetQuestObjective 는 Wait 태스크와 같은 상태에 얹혀 `bConsideredForCompletion=false`(끼면 그 상태가 영영 완료 안 됨). 둘 다 완료를 내지 않고 상주하며, 상태 완료는 짝이 되는 Wait 태스크가 낸다.
- **저널 정리는 노드가 아님** — 트리 종료(완료·실패·교체 세 경로)를 컴포넌트가 러너 RunStatus 콜백 한 곳에서 감지해 자동 정리한다.
- **러너 재시작은 다음 틱** — ST 실행 콜스택 안 재진입 금지. 콜스택 안 요청은 `RequestActivateQuest` 로 다음 틱 예약.
- **레벨 액터 지정**: `FWxActorTarget`(내부 FUniversalObjectLocator)로 배치 액터 직접 지정, 매 틱 SyncFind(강제 로드·캐시 없음). 러너가 이미 권위 전용이니 기믹 노드의 권위/스킵 게이트를 복사하지 말 것.

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 러너 권위 구동·저널·수주 경로가 응축된 시스템 관문
2. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` — 네 노드의 계약과 저널 태스크 완료 판정 함정
3. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 권위 게이트·다음 틱 시작·RunStatus 콜백 정리의 실제 흐름

## 관련
- 상위: [[WxCore]] (`FWxActorTarget`)
- 소비: [[WxUI]] (저널 델리게이트 구독)

---
*문서 기준 커밋 `28ee2c6` · 생성일 2026-08-03 · 소스 9파일 — `/readme-writer`로 갱신*
