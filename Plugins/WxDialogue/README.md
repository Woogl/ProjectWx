# WxDialogue — 대화 시스템

> NPC·오브젝트에게 말을 걸어 데이터 테이블에 담긴 대사를 한 줄씩 넘겨 재생하고, 그 동안 전용 카메라·포즈 연출을 세우는 대화 진행 시스템. 대사의 신원(현재 행)만 노출하고 그 의미 해석은 소비자에게 맡긴다.

## 책임
**담당**
- 대화 데이터 스키마(대사 한 줄 = 테이블 한 행, `NextRow` 로 잇고 `None` 이면 종료)
- 상호작용으로 대화를 열고, 대사를 넘기고(`Advance`), 끝맺는 세션 진행
- 대화 시작 지점 보유(호스트 액터)와 세션 소유(플레이어 컨트롤러)의 분리
- 대화 연출: 두 사람 배치에서 구도를 잡는 전용 카메라, 대사별 대상 포즈 몽타주 비동기 스트리밍
- 진행 중 대사의 신원(현재 행)·대상 노출과 대사 변경 델리게이트(`OnLineChanged`) 발행
- 퀘스트 등 액터 외부가 대사를 고르는 진입점(StateTree `Play Dialogue` 태스크)

**경계 (비담당)**
- 상호작용 트리거·프롬프트 판정 계약(`IWxInteractable`) — [[WxCore]]가 정의, 호스트 액터가 구현만
- 세션 개폐를 알리는 `State.Dialogue` 태그와 ASC — [[WxCore]]의 `WxGameplayTags`, 태그를 보는 UI 창 개폐 — [[WxUI]]
- 대사의 의미(퀘스트 수주 등) 판정 — 관찰하는 소비자([[WxQuest]] 등)의 몫

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터 한 행(화자·대사·포즈·다음 행). 대화 1편 = 테이블 1개 | `Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트. `IWxInteractable` 계약을 컴포넌트로 넘김 | `Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 호스트에 붙어 시작 행·프롬프트만 보유, 상호작용 시 세션을 연다 | `Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | 진행·카메라·포즈를 소유하는 세션 본체. PC에 Experience로 주입 | `Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 액터 없이 행을 직접 지정해 대화를 여는 StateTree 태스크(독백·무전·처치 후 대사) | `Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화 대상: `AWxDialogueActor`를 상속해 몸통(캡슐+스켈레탈 / 메시)을 세운다. 베이스는 루트를 만들지 않으며, 포즈를 얹을 메시가 있으면 `GetPoseMesh()`를 재정의한다.
- 대화 편성: `FWxDialogueTableRow` 로우 타입의 데이터 테이블로 작성한다. 모든 행이 `NextRow`를 채워야 하며 종료는 `None`으로 표시한다(빈 값·빈 대사는 잘못된 행으로 보고 경고 후 대화를 접는다). 포즈는 `TSoftObjectPtr` 소프트 참조로, 세션이 대사를 넘길 때 비동기 스트리밍한다.
- 외부에서 대사 열기: `UWxDialogueSessionComponent::StartDialogueRow(StartRow, Target)`. `Target`을 비우면 나레이션(카메라는 플레이어에 머묾).
- 전제: 세션은 소유 클라 로컬 표시 상태이며 서버 검증이 없다(v1 싱글/리슨 호스트). 폰 ASC가 없으면 `State.Dialogue` 태그를 올릴 곳이 없어 세션을 열지 않는다.

## 여기서부터 읽어라
1. `Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 소유·복제·카메라·포즈·태그 정책이 헤더 주석에 모두 서술된 시스템의 심장부
2. `Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화가 어떤 데이터로 표현되는지, 진행 규약(NextRow/None)의 출발점
3. `Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — `EnterRow`→`PublishCurrentLine`→`Advance`→`EndDialogue` 진행과 카메라·포즈 스트리밍 실제 흐름

## 관련
- 상위: 호스트 액터를 상호작용으로 부르는 [[WxCore]]의 `IWxInteractable`, 세션을 PC에 주입하는 Experience, 진행 대사를 관찰해 의미를 판정하는 [[WxQuest]], `State.Dialogue` 태그로 창을 여닫는 [[WxUI]]

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 11파일 — `/readme-writer`로 갱신*
