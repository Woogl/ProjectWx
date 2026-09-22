---
title: "WxDialogue — 대화 세션"
category: topic
sources:
  - "raw/notes/2026-09-22-current-dialogue.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, dialogue]
aliases: ["WxDialogue"]
confidence: medium
volatility: warm
summary: "대화 정의는 액터에, 진행 세션은 PlayerController에 두고 행 데이터·ASC 태그·UI를 연결한다."
---

# WxDialogue — 대화 세션

대화 정의는 액터에, 진행 세션은 PlayerController에 두고 행 데이터·ASC 태그·UI를 연결한다.

## 책임과 데이터 흐름

`UWxDialogueComponent`의 시작 행을 세션에 전달하면 `ClientStartDialogue`가 소유 클라이언트에서 대화를 연다. 세션은 현재 DataTable·행·대상을 보관하고 `Advance`로 NextRow를 따라간다. NextRow가 None이면 정상 완료이고, 누락된 행이나 빈 대사는 실패·중단 경로다. 테이블을 수정할 때 종료 행과 오타를 혼동하지 않는다.

진행 중인 세션에 새 대화가 들어오면 이전 세션을 먼저 중단한다. Pawn ASC가 없으면 새 세션을 열지 않는다. 유효한 행을 채운 뒤 `State.Dialogue` loose 태그를 1로 설정하므로 UI가 태그 변화를 받아 즉시 현재 대사를 읽을 수 있다. 종료는 기억한 ASC의 카운트를 0으로 돌리고 카메라를 복구한다.

## 수명과 연출

빙의가 바뀌면 대화는 미완료로 종료된다. 종료 통지는 델리게이트를 사본으로 옮기고 원본을 비운 뒤 발행한다. 종료 콜백에서 새 대화를 여는 경우 새 구독을 지우지 않기 위한 순서다.

카메라는 로컬 플레이어·Pawn·대상이 있는 경우 임시 CameraActor로 블렌드한다. 복귀 블렌드가 끝나도록 카메라에 수명을 준다. 대상 포즈는 비동기 로드하며 새 포즈 요청 때 이전 핸들을 취소한다. 세션 종료는 마지막 포즈의 로딩을 취소하지 않으므로 대화 종료와 포즈 재생 수명은 같지 않다.

## StateTree 연결의 한계

`PlayDialogue` 태스크는 0번 PlayerController의 세션을 찾고 시작 직후 `HasActiveDialogue`를 확인한다. 정상 완료 통지를 받아야 Succeeded로 끝난다. 소유 클라이언트와 권위가 같은 머신인 싱글플레이·리슨 호스트 전제가 드러나는 경로다. 원격 클라이언트의 진행 상태를 서버가 검증·완료 처리하는 퀘스트 프로토콜로 간주하지 않는다.

진입점: [세션 구현](../../../Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp), [StateTree 태스크](../../../Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp). 대사 테이블·포즈 에셋·원격 완료는 별도 확인이 필요하다.

## 관련 문서

- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-dialogue.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. 인간 검증일 `verified`는 새로 부여하지 않았다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
