---
title: "Dialogue 화면 수명으로 연결 책임 통합"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, dialogue, lifecycle]
summary: "사용자 승인으로 Dialogue Resolver를 제거하고 화면 활성화·비활성화가 세션 구독과 표시 갱신을 소유한다."
---

# Dialogue 화면 수명으로 연결 책임 통합

2026-09-23 사용자 승인과 미커밋 코드 정적 확인이다. 같은 날의 `2026-09-23-dialogue-presentation-vm.md` 중 Resolver가 연결을 소유한다는 설명을 대체한다.

- `Source/WxGame/UI/WxDialogueScreen.h/.cpp`: 화면 활성화 시 세션 OnLineChanged를 WxUI Dialogue VM의 SetLine에 연결하고 현재 대사로 채운다. 비활성화·파괴 시 해제한다. 관찰 세션과 표시 VM은 화면에 약한 참조로 보관한다. 진행 입력은 활성 화면의 관찰 세션에 전달한다.
- `Content/UI/Widget/WBP_DialogueScreen.uasset`: VM 생성 방식은 Create Instance, Resolver 참조는 null이다. 표시 바인딩과 Self.RequestAdvance 이벤트는 유지한다.
- `Source/WxGame/MVVM/WxViewModelResolver_Dialogue.h/.cpp`는 제거했다. 순수 표시 VM은 WxUI에 유지하고 WxCore·도메인 모듈 의존성은 추가하지 않는다.
- 엔진 `UUserWidget::NativeConstruct`는 확장의 Construct를 실행한다. `UMVVMView::Construct`는 설정에 따라 소스와 바인딩을 초기화한다. `UCommonActivatableWidget::NativeConstruct`는 그 뒤 자동 활성화한다. 이 순서로 VM 생성 후 연결하며, 생성 전에 활성화된 경우는 화면 NativeConstruct 완료 후 연결한다.
- 다시 활성화할 때 현재 대사를 읽어 비활성 기간에 놓친 변경을 복구한다. 위젯 재생성 시 MVVM이 새 VM을 만들고 화면이 다시 연결한다.

구체적인 빌드·수명 테스트와 인게임 검증 범위는 [작업 기록](../../../.agents/workflow/tasks/dialogue-presentation-vm.md)에 기록한다. 이 원자료의 엔진 순서 설명은 로컬 UE 5.8 소스 확인이다.
