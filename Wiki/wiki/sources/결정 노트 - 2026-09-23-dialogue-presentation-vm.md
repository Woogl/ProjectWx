---
type: source
title: "결정 노트 - 2026-09-23-dialogue-presentation-vm"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "MVVM"
  - "대화"
summary: "Dialogue VM을 WxUI의 순수 표시 데이터로 만들고 세션 연결은 WxGame Resolver, 진행 입력은 화면이 맡게 한 모듈 경계 결정과 정적 확인 기록."
source_type: decision-note
source_id: src-fc63937bb42b710abf3f
sha256: a8e987aff16a7d77c8fd03f2217f4cad69155d03ddb6a7114127f0fecdffec18
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-dialogue-presentation-vm.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-dialogue-presentation-vm.md"
raw_copy: ".raw/captured/a8e987aff16a7d77c8fd03f2217f4cad69155d03ddb6a7114127f0fecdffec18.md"
claim_ids:
  - clm-97db594099-c1
  - clm-97db594099-c2
  - clm-97db594099-c3
key_claims:
  - "2026-09-23 사용자 확정 원칙은 Wx 기능 모듈 사이 의존성 추가를 WxCore 외에는 금지하고 조립 계층 WxGame이 도메인과 화면을 연결하게 한다."
  - "2026-09-23 정적 확인 기준 WxViewModel_Dialogue는 Speaker·LineText·HasSpeaker 표시 필드만 갖고 세션 참조·구독·진행 함수를 갖지 않는다."
  - "Dialogue VM 분리 작업의 인게임 화면·클릭·재진입·빙의 변경 동작은 이 노트 시점에 검증되지 않았다."
---

# 결정 노트 - 2026-09-23-dialogue-presentation-vm

- 원본: `.wiki/raw/notes/2026-09-23-dialogue-presentation-vm.md`
- 원자료 사본: `.raw/captured/a8e987aff16a7d77c8fd03f2217f4cad69155d03ddb6a7114127f0fecdffec18.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-dialogue-presentation-vm.md` (source: MANUAL, ingested: 2026-09-23).
- 2026-09-23 사용자 요청과 당시 미커밋 구현의 정적 확인 기록이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다. 특히 같은 날의 후속 노트 [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]]가 `WxDialogueScreen` 클래스를 제거하고 진행 입력을 VM 명령으로 옮겼으므로, 이 노트의 화면 클래스 설명은 이후 대체되었다.

## 사람의 판단(노트의 사용자 확정 원칙 원문)

> 사용자 2026-09-23(노트 기록): "Wx 기능 모듈 사이의 의존성 추가는 WxCore를 제외하면 금지한다. 그렇다고 WxCore에 직접 게임 로직을 넣거나 공용 계약에 과도하게 의존하는 구조를 원하지 않는다. 조립 계층 WxGame이 기존 의존성으로 도메인과 화면을 연결한다. Dialogue VM 자체를 순수 표시 데이터로 만들며 자식 VM을 추가하는 방식은 아니다."

## 구현 관찰(정적 확인)

- `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Dialogue.h`와 같은 이름의 cpp: Speaker, LineText, HasSpeaker, SetLine 변경 알림만 있고 세션 참조·구독·진행 함수가 없다.
- `Source/WxGame/MVVM/WxViewModelResolver_Dialogue.cpp`: 세션 `OnLineChanged`를 VM의 `SetLine`에 직접 연결하고 현재 값으로 초기화한다. 구독한 세션을 VM의 Outer로 남겨 해제한다. 뷰별 가변 상태가 없고, 세션이 없으면 빈 VM을 만든다.
- `Source/WxGame/UI/WxDialogueScreen.cpp`: `RequestAdvance`가 위젯 소유 컨트롤러의 세션 `Advance`를 호출한다.
- `WBP_DialogueScreen`: 부모 `WxDialogueScreen`, `AdvanceButton.OnClicked`의 MVVM 이벤트 목적지 `Self.RequestAdvance`.
- `Config/DefaultEngine.ini`: VM의 WxGame → WxUI ClassRedirect. Build.cs·WxCore·WxDialogue 소스 변경 없음, 새 연결 객체·자식 VM 없음.

## 검증 범위

- WxEditor Development 빌드 성공.
- 저장한 WBP를 새 프로세스로 로드·컴파일(경고도 실패 처리)하고, 세션 신호의 두 VM 전달·개별 해제·화자 없음·빈 대사를 확인했다(종료 코드 0).
- 미검증: Resolver 생성·해제 전체, 인게임 화면·클릭·재진입·빙의 변경.

## 관련 주제

- [[UI 표시 구조]]
- [[퀘스트와 대화]]
- [[모듈 구조와 코드 정리]]
- [[작업 - dialogue-presentation-vm]]

## 핵심 주장

- 2026-09-23 사용자 확정 원칙은 Wx 기능 모듈 사이 의존성 추가를 WxCore 외에는 금지하고 조립 계층 WxGame이 도메인과 화면을 연결하게 한다. ^c1
- 2026-09-23 정적 확인 기준 WxViewModel_Dialogue는 Speaker·LineText·HasSpeaker 표시 필드만 갖고 세션 참조·구독·진행 함수를 갖지 않는다. ^c2
- Dialogue VM 분리 작업의 인게임 화면·클릭·재진입·빙의 변경 동작은 이 노트 시점에 검증되지 않았다. ^c3
