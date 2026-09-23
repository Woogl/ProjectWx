---
title: "WxUI — 화면 레이어와 표시 수명"
category: topic
sources:
  - "raw/notes/2026-09-23-quest-presentation-vm.md"
  - "raw/notes/2026-09-22-current-ui.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-player-screen-owner.md"
  - "raw/notes/2026-09-23-ability-resolver-module.md"
  - "raw/notes/2026-09-23-ability-resolver-colocation.md"
  - "raw/notes/2026-09-23-dialogue-presentation-vm.md"
  - "raw/notes/2026-09-23-dialogue-screen-lifecycle.md"
created: 2026-09-22
updated: 2026-09-23
tags: [wx, ui]
aliases: ["WxUI"]
confidence: medium
volatility: warm
verified: 2026-09-23
summary: "WxUI는 CommonUI 레이어와 MVVM 표시를 관리하고, 도메인 상태는 공용 태그·표시 계약으로 관찰한다."
---

# WxUI — 화면 레이어와 표시 수명

WxUI는 CommonUI 레이어와 MVVM 표시를 관리하고, 도메인 상태는 공용 태그·표시 계약으로 관찰한다.

## 화면과 상태의 연결

`UWxUIManagerSubsystem`은 GameInstance 수명으로 기본 레이아웃·확인 팝업·일시정지를 관리한다. `UWxPrimaryGameLayout`은 태그별 위젯 스택을 가지며 LayerTags 배열 순서가 z-order다. Game·GameMenu·Menu·Modal을 같은 의미로 취급하지 않는다. 메뉴 활성 판정은 Menu/Modal이고 GameMenu는 포함하지 않는다.

사망 태그는 Menu에 사망 화면을 띄우고, `State.Dialogue`는 Game에 대화 화면을 올린다. 이 관찰과 화면 클래스의 주인은 PlayerController의 `UWxPlayerLayoutComponent`다. 전역 `UWxUIDeveloperSettings`에는 UI의 틀(레이아웃·확인 팝업)만 두고, 게임 화면은 컨트롤러 BP에서 지정해 모드별로 바꿀 수 있게 한다. 태그가 먼저 사라지면 진행 중인 비동기 대화 화면 요청을 취소한다. 로드 완료 콜백에서도 현재 태그를 재확인해 이미 끝난 대화 창이 뒤늦게 나타나는 것을 막는다.

## 폰 교체와 구독 해제

PlayerController의 `UWxPlayerLayoutComponent`는 로컬 컨트롤러에서만 화면을 만든다. 폰 교체 때 기존 HUD와 비동기 요청을 정리하고 새 폰 기준으로 다시 생성한다. ViewModel이 생성 당시 Pawn의 ASC를 참조하므로 폰만 바꾸고 HUD를 남겨서는 안 된다.

폰 교체 때 태그 관찰도 새 폰으로 갈아타며, 대화 창은 닫지만 사망 화면은 닫지 않는다. 부활이 폰을 교체하고, 사망 화면은 부활 요청이 완료될 때 스스로 비활성화되기 때문이다.

Attribute ViewModel은 초기 값을 읽은 뒤 속성 변경을 구독하고, Deinitialize에서 같은 ASC의 구독과 캐시를 해제한다. 도메인별 인벤토리·대화·퀘스트 ViewModel의 조립은 WxGame에 있다. 표시 값 접근은 `IWxUIData`와 GAS 계약을 사용한다.

Wx 기능 모듈 사이의 신규 의존성은 WxCore를 제외하면 추가하지 않는다. WxCore도 공용 정의 범위를 유지하며 UI 이동을 위해 게임 로직이나 과도한 중계 계약을 넣지 않는다. 도메인 연결은 WxGame의 조립 책임이다.

Dialogue VM 자체는 WxUI의 순수 표시 데이터다. Speaker·LineText·HasSpeaker와 SetLine 변경 알림만 남기고, WBP는 MVVM의 Create Instance로 생성한다. WxGame의 WxDialogueScreen이 활성화 때 세션 구독·현재 대사 동기화를 수행하고 비활성화·파괴 때 해제한다. 다시 활성화하면 현재 대사로 표시를 복구한다. 클릭 이벤트는 Self.RequestAdvance로 연결해 화면이 처리한다. 별도 Resolver·자식 VM·연결 객체는 없다. 최종 검증 범위는 [작업 기록](../../../.agents/workflow/tasks/dialogue-presentation-vm.md)에 있으며 인게임 화면·클릭은 별도 확인 대상이다.

Quest·QuestObjective VM도 WxUI의 표시 데이터다. WxGame의 QuestTracker가 일반 위젯의 생성·해제 수명에 저널 구독을 연결한다. Quest는 Create Instance로 생성하며, 목표별 행 VM은 Quest VM이 소유한다. 자세한 계약은 [퀘스트](quests.md)의 조립과 범위를 참고한다.

## 일시정지와 제약

`UWxViewModelResolver_Ability`는 WxUI의 `WxViewModel_Ability.h/.cpp`에 함께 둔다. 위젯 소유 컨트롤러의 Pawn에서 ASC를 얻고, AbilityTags에 대응하는 공유 슬롯 VM을 AbilitySystem VM에서 가져온다. 이전 WxGame 클래스 경로는 CoreRedirect로 유지한다. 이는 모듈 이동의 정적 확인이며 기존 WBP 로드·표시 검증과는 별개다.

활성 `UWxActivatableWidget`의 ShouldPauseGame 요청을 보고 Standalone에서만 정지를 조정한다. 메뉴가 있다는 사실만으로 멀티플레이 월드를 정지하지 않는다. 레이아웃과 추적 PC는 단수이므로 현재 구조는 로컬 플레이어 하나를 전제로 하며 스플릿스크린 지원으로 해석하지 않는다.

진입점: [UIManager](../../../Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp), [HUD·사망·대화 화면 수명](../../../Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp), [설정](../../../Config/DefaultGame.ini). 화면 클래스 값은 `BP_PlayerController`에 있다. 사망·부활·대화 화면 동작은 2026-09-23 사용자가 인게임에서 확인했고, 그 밖의 WBP 바인딩과 화면 품질은 에디터·실행 확인이 필요하다.

## 관련 문서

- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))

## Sources

- [Quest 표시 VM 분리](../../raw/notes/2026-09-23-quest-presentation-vm.md)

- [근거 1](../../raw/notes/2026-09-22-current-ui.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [근거 3](../../raw/notes/2026-09-23-player-screen-owner.md)
- [근거 4](../../raw/notes/2026-09-23-ability-resolver-module.md)
- [근거 5](../../raw/notes/2026-09-23-ability-resolver-colocation.md)
- [근거 6](../../raw/notes/2026-09-23-dialogue-presentation-vm.md)
- [근거 7](../../raw/notes/2026-09-23-dialogue-screen-lifecycle.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
