---
title: "Dialogue VM 순수 표시 데이터 분리와 모듈 경계"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, dialogue, architecture]
summary: "Dialogue VM은 WxUI 표시 데이터로, 세션 연결과 진행 입력은 WxGame Resolver와 화면으로 분리했다."
---

# Dialogue VM 순수 표시 데이터 분리와 모듈 경계

2026-09-23 사용자 요청 및 미커밋 구현의 정적 확인이다.

## 사용자 확정 원칙

Wx 기능 모듈 사이의 의존성 추가는 WxCore를 제외하면 금지한다. 그렇다고 WxCore에 직접 게임 로직을 넣거나 공용 계약에 과도하게 의존하는 구조를 원하지 않는다. 조립 계층 WxGame이 기존 의존성으로 도메인과 화면을 연결한다. Dialogue VM 자체를 순수 표시 데이터로 만들며 자식 VM을 추가하는 방식은 아니다.

## 구현 근거

- `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Dialogue.h` 및 Private의 같은 cpp: Speaker, LineText, HasSpeaker, SetLine 변경 알림만 있다. 세션 참조·구독·진행 함수는 없다.
- `Source/WxGame/MVVM/WxViewModelResolver_Dialogue.cpp`: 세션 OnLineChanged를 VM의 SetLine에 직접 연결하고 현재 값으로 초기화한다. 구독한 세션을 VM의 Outer로 남겨 실제 원본에서 해제한다. Resolver에는 뷰별 가변 상태가 없다. 세션이 없으면 빈 VM을 만든다.
- `Source/WxGame/UI/WxDialogueScreen.cpp`: RequestAdvance는 위젯 소유 컨트롤러의 세션 Advance를 호출한다.
- `Content/UI/Widget/WBP_DialogueScreen.uasset`: 부모 WxDialogueScreen, AdvanceButton.OnClicked의 MVVM 이벤트 목적지 Self.RequestAdvance. 표시 필드 바인딩은 같은 VM을 사용한다.
- `Config/DefaultEngine.ini`: VM의 WxGame → WxUI ClassRedirect. Resolver 클래스는 WxGame 경로 유지.
- Build.cs·WxCore·WxDialogue 소스에 변경 없음. 새로운 연결 객체나 자식 VM 없음.

## 검증 범위

WxEditor Development 빌드 성공. 저장한 WBP를 새 프로세스로 로드·컴파일(경고도 실패로 처리)하고 세션 신호의 두 VM 전달·개별 해제·화자 없음·빈 대사를 확인했다. 최종 검증 종료 코드 0. Resolver 생성·해제 전체와 인게임 화면·클릭·재진입·빙의 변경은 미검증이다. 구체적인 로그와 상태는 [작업 기록](../../../.agents/workflow/tasks/dialogue-presentation-vm.md)에 있다.
