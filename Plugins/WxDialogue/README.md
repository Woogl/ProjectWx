# WxDialogue — 대화 시스템

> NPC·오브젝트에 말을 걸어 대사 테이블을 한 줄씩 진행하고, 대화 동안 전용 카메라와 대상 포즈를 연출한다. 대사의 신원(현재 행)과 대상만 노출하고 그 의미 판정은 관찰자에게 맡긴다.

## 책임
**담당**
- 대화 데이터 모델: 행 하나 = 대사 한 줄, `NextRow`로 이어지고 `None`에서 끝나는 테이블 주도 그래프
- 상호작용 진입: 말을 걸 수 있는 호스트 액터가 상호작용 계약을 받아 대화 컴포넌트로 넘김
- 세션 진행: 소유 클라(PlayerController)가 현재 노드·라인을 소유하고 `Advance`로 넘기며, 대사 변화를 델리게이트로 발행
- 연출: 대화 전용 카메라 구도 계산·뷰 전환, 대사별 대상 포즈(몽타주) 비동기 스트리밍·재생
- 세션 개폐 신호: 폰 ASC에 `State.Dialogue` 루즈 태그를 올렸다 내림
- 퀘스트 등 액터 밖에서 특정 대사를 여는 StateTree 태스크 진입점

**경계 (비담당)**
- 대화 창 UI 렌더링 — `State.Dialogue` 태그를 보는 [[WxUI]]가 여닫음. 이 모듈은 창 시작·종료 델리게이트를 두지 않음
- 대사의 의미 해석(퀘스트 수주 등)·대화 기록 — 진행 대사를 관찰하는 소비자([[WxQuest]] 등)의 몫
- 상호작용 계약 인터페이스 `IWxInteractable`·`State.Dialogue` 태그 정의 — [[WxCore]] 제공

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 그래프의 행(대사·화자·포즈·NextRow). 대화 1편 = 테이블 1개 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 Abstract 호스트. 상호작용 계약을 받아 컴포넌트로 위임 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 호스트에 붙어 시작 행만 보유. `StartDialogueWith`로 상호작용자 세션을 엶 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC에 주입되는 세션 소유자. 진행·카메라·포즈·델리게이트를 모두 든 중심 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 액터 밖(퀘스트 ST)에서 지정 대사를 열고 종료까지 Running으로 대기 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화: `FWxDialogueTableRow` 로우 타입 DataTable을 만들고 `UWxDialogueComponent::StartRow`(또는 ST 태스크의 `StartRow`)로 시작 행을 지정. 모든 행은 `NextRow`를 채워야 하며 종료는 `NextRow=None`
- 새 말 거는 대상: `AWxDialogueActor`를 상속해 몸통(캡슐+스켈레탈/메시)을 세우고, 포즈를 얹으려면 `GetPoseMesh()`를 오버라이드
- 포즈는 소프트 참조(`TSoftObjectPtr<UAnimMontage>`)로, 세션이 대사를 넘길 때 비동기 스트리밍. 대화가 끝나도 되돌리지 않고 마지막 자세로 남음
- 권한 모델: 서버 권위 진입점(`StartDialogue`)이 Client RPC로 소유 클라를 열고, 세션 진행 상태는 로컬 표시 전용(서버 검증 없음). v1 싱글/리슨 호스트 전제
- 세션 전제: 폰 ASC가 없으면 `State.Dialogue`를 올릴 곳이 없어 세션을 열지 않음. 카메라는 로컬 컨트롤러에서만 동작(나레이션이면 카메라 미개입)

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 진행·연출·복제 정책이 클래스 주석에 응축된 모듈의 심장
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화 데이터 모델. 행→NextRow 그래프 규약
3. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — 카메라 구도 계산·포즈 스트리밍·태그 개폐의 실제 흐름

## 관련
- 상위: 호스트 액터를 상호작용으로 부르는 [[WxCore]]의 `IWxInteractable`, 세션을 PC에 주입하는 Experience, 진행 대사를 관찰해 의미를 판정하는 [[WxQuest]], `State.Dialogue` 태그로 창을 여닫는 [[WxUI]]

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 11파일 — `/readme-writer`로 갱신*
