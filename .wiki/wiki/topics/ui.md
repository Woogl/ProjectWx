---
title: "WxUI — 화면 레이어와 표시 수명"
category: topic
sources:
  - "raw/notes/2026-09-22-current-ui.md"
  - "raw/notes/2026-09-22-current-foundation.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, ui]
aliases: ["WxUI"]
confidence: medium
volatility: warm
verified: 2026-09-22
summary: "WxUI는 CommonUI 레이어와 MVVM 표시를 관리하고, 도메인 상태는 공용 태그·표시 계약으로 관찰한다."
---

# WxUI — 화면 레이어와 표시 수명

WxUI는 CommonUI 레이어와 MVVM 표시를 관리하고, 도메인 상태는 공용 태그·표시 계약으로 관찰한다.

## 화면과 상태의 연결

`UWxUIManagerSubsystem`은 GameInstance 수명으로 기본 레이아웃을 관리한다. `UWxPrimaryGameLayout`은 태그별 위젯 스택을 가지며 LayerTags 배열 순서가 z-order다. Game·GameMenu·Menu·Modal을 같은 의미로 취급하지 않는다. 메뉴 활성 판정은 Menu/Modal이고 GameMenu는 포함하지 않는다.

사망 태그는 Menu에 사망 화면을 띄우고, `State.Dialogue`는 Game에 대화 화면을 올린다. 태그가 먼저 사라지면 진행 중인 비동기 대화 화면 요청을 취소한다. 로드 완료 콜백에서도 현재 태그를 재확인해 이미 끝난 대화 창이 뒤늦게 나타나는 것을 막는다.

## 폰 교체와 구독 해제

PlayerController의 `UWxPlayerLayoutComponent`는 로컬 컨트롤러에서만 HUD를 만든다. 폰 교체 때 기존 HUD와 비동기 요청을 정리하고 새 폰 기준으로 다시 생성한다. ViewModel이 생성 당시 Pawn의 ASC를 참조하므로 폰만 바꾸고 HUD를 남겨서는 안 된다.

Attribute ViewModel은 초기 값을 읽은 뒤 속성 변경을 구독하고, Deinitialize에서 같은 ASC의 구독과 캐시를 해제한다. 도메인별 인벤토리·대화·퀘스트 ViewModel의 조립은 WxGame에 있다. 표시 값 접근은 `IWxUIData`와 GAS 계약을 사용한다.

## 일시정지와 제약

활성 `UWxActivatableWidget`의 ShouldPauseGame 요청을 보고 Standalone에서만 정지를 조정한다. 메뉴가 있다는 사실만으로 멀티플레이 월드를 정지하지 않는다. 레이아웃과 추적 PC/ASC는 단수이므로 현재 구조는 로컬 플레이어 하나를 전제로 하며 스플릿스크린 지원으로 해석하지 않는다.

진입점: [UIManager](../../../Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp), [HUD 수명](../../../Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp), [설정](../../../Config/DefaultGame.ini). 실제 WBP 바인딩과 화면 품질은 에디터·실행 확인이 필요하다.

## 관련 문서

- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-ui.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
