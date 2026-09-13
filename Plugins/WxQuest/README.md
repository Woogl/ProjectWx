# WxQuest — 퀘스트 시스템

> StateTree 에셋으로 기술된 퀘스트를 서버 권위로 실행하고, 활성 퀘스트의 제목·목표 저널을 관리해 HUD에 알린다. 퀘스트 1개 = UStateTree 에셋 1개, 활성 퀘스트는 동시 1개(새 시작은 교체)다.

## 책임
**담당**
- 퀘스트 StateTree 러너의 권위 측 실행·수명 관리, 그리고 활성 퀘스트의 저널(제목·목표) 상태 보관
- 저널 조작을 위한 StateTree 태스크 노드 제공(제목 설정, 목표 추가/제거, 다음 퀘스트 예약, 목표 지점 도달 대기)
- 저널 변경 통지(HUD 뷰모델이 구독해 pull)

**경계 (비담당)**
- 저널을 화면에 그리는 것 — [[WxUI]]
- 퀘스트 완료 시 보상·월드 부수효과 — 별도 StateTree 노드/타 모듈에 위임
- 배치 액터 로케이터 해석 유틸 — [[WxCore]] (`WxLocatorUtils`)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 기본 서브오브젝트. 권위 측에서 러너를 소유하고 저널을 관리하는 중심. 태스크·라이브러리가 여기로 수렴 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 레벨 배치물(트리거 볼륨 등)이 퀘스트를 수주시키는 외부 진입점. GameState에서 컴포넌트를 찾아 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 상태 진입 시 저널 제목 등록 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 동안 목표 하나를 걸고 이탈 시 핸들로 걷어감 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 로케이터 대상 도달까지 상태 완료를 붙잡는 대기 태스크 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱에 예약해 체인 연결 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- 새 퀘스트 스텝/조건은 `FStateTreeTaskCommonBase` 파생 USTRUCT로 추가한다. 기존 태스크가 표준 형태다: `using FInstanceDataType` + 헤더의 `GetInstanceDataType()`(코딩 규칙 4 예외), 파라미터는 InstanceData의 `EditAnywhere` 필드로.
- 저널을 바꾸는 태스크는 컨텍스트 오너(GameState)에서 `UWxQuestComponent`를 찾아 `SetQuestTitle`/`AddObjective`/`RemoveObjective`로 위임한다. 상태 완료는 이 태스크들이 아니라 짝이 되는 Wait 태스크가 낸다(제목·목표 태스크는 완료 판정에서 빠져 있음).
- 저널 정리는 태스크가 아니라 러너의 실행 상태 변경 통지 한 곳으로 수렴한다(완료·실패·교체 세 경로 공통).
- 러너 실행 콜스택 안에서의 재시작은 엔진 재진입 가드에 막히므로, 콜스택 안 활성화는 `RequestActivateQuest`로 다음 틱 예약한다.
- 배치 액터 지정은 `FUniversalObjectLocator`를 쓴다(순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증을 우회, 씬 픽커·WP·PIE 해석은 엔진 내장).

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 doc-comment가 아키텍처 전체(러너 권위 소유, 저널 수렴, 에셋 불가지)를 설명한다. 여기부터.
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 런타임 생성·위임·상태 변경 통지 처리의 실제 구현.
3. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` — 퀘스트가 어떻게 수주되는지(외부→컴포넌트) 흐름의 시작점.

## 관련
- 상위: 퀘스트 수주는 레벨 배치물이 [[WxWorld]] 상호작용을 통해 `UWxQuestLibrary::StartQuest`를 호출하는 경로, 저널 표시는 [[WxUI]] 뷰모델. 부착 지점인 GameState는 [[WxGame]].
- 하위: 로케이터 해석 등 공용 유틸은 [[WxCore]].

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 14파일 — `/readme-writer`로 갱신*
