---
title: "사망·대화 화면 주인을 컨트롤러 컴포넌트로 이동"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, static-review]
summary: "사망·대화 화면 클래스와 태그 관찰을 UIManager 서브시스템·전역 설정에서 UWxPlayerLayoutComponent로 옮긴 결정과 코드 근거. 사용자가 인게임 동작을 확인했다."
revision: 47b7f8bd7
---

# 사망·대화 화면 주인을 컨트롤러 컴포넌트로 이동

2026-09-23 HEAD `47b7f8bd7` 및 미커밋 작업 트리의 정적 조사와 사용자 결정 기록이다. 작업 상태·빌드 근거는 [Workflow Task](../../../.agents/workflow/tasks/player-screen-classes-to-layout-component.md)에 있다.

## 결정 (사용자 확정)

- `UWxUIDeveloperSettings`에는 UI의 틀(`LayoutClass`, `ConfirmationPopupClass`)만 둔다. 게임플레이에 반응하는 화면은 컨트롤러 BP의 `UWxPlayerLayoutComponent`에 둔다.
- 근거: 사망·대화 화면은 `a8cffba6c`(2026-07-28)에서 HUD와 함께 설정으로 옮겨졌다. 이후 HUD만 `f8718e3a6`(2026-08-23)에서 컴포넌트로 돌아갔고, 이 둘은 설정에 남아 있었다.
- 효과: 모드마다 컨트롤러 BP를 달리하면 다른 화면을 쓸 수 있다. 서브시스템은 레이아웃·팝업·일시정지만 맡는다. 값이 ini에서 BP 에셋으로 옮겨져 git diff로는 리뷰할 수 없다.

## 확인한 계약

- 컴포넌트는 폰이 바뀔 때(`OldPawn != NewPawn`) 새 폰의 `Ability.Death`·`State.Dialogue` 태그 관찰로 갈아타고, `EndPlay`에서 관찰을 끊는다.
- 관찰을 갈아탈 때 대화 창은 닫지만 사망 화면은 닫지 않는다. 부활이 폰을 교체하며, 사망 화면은 부활 요청이 완료될 때 스스로 비활성화된다.
- 빈 클래스 판정은 `UWxAsyncAction_PushWidgetToLayer`가 한다. 값이 비어 있으면 해당 화면을 띄우지 않는다.
- `UWxUIManagerSubsystem`의 `TrackedPlayerController`는 이제 일시정지에만 쓰이며, 빙의를 구독하지 않는다.
- 값: `BP_PlayerController`에 `WBP_DeathScreen`·`WBP_DialogueScreen`이 들어 있다. `BP_FrontEndPlayerController`는 비어 있다. 사용자가 사망·부활·대화 동작을 인게임에서 확인했다(2026-09-23).

## 코드 근거

| 파일 | 확인 범위 | SHA-256 |
|---|---|---|
| [WxPlayerLayoutComponent.h](../../../Plugins/WxUI/Source/WxUI/Public/Component/WxPlayerLayoutComponent.h) | 화면 클래스·관찰 상태 | `30f8ecbc1c9e87c829aa53ef177e1c75221f6f547dd6bbac25f0060f594d18fc` |
| [WxPlayerLayoutComponent.cpp](../../../Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp) | 폰 교체·태그 관찰·대화 창 수명 | `f7cefdf1ea0f0ab55fe3740c9ed08c832e3baff24d2f2c27fd7a68bde7c88c74` |
| [WxUIManagerSubsystem.h](../../../Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h) | 남은 책임 | `7e72d733e46cd80670e6ab754599a6f13c88140454172384c7d0001edb651884` |
| [WxUIManagerSubsystem.cpp](../../../Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp) | PC 교체·일시정지 | `c06c9193db897244f297886be2eea2e52c7880c9ae21e10a505d52a0b170f36d` |
| [WxUIDeveloperSettings.h](../../../Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h) | 설정 필드 | `c8cce63b230611505d5dfc7ba728be7ac14a6d9bd680f0ec86cce0e21cc8037a` |
| [DefaultGame.ini](../../../Config/DefaultGame.ini) | WxUIDeveloperSettings 섹션 | `fde49745e41f0653039e05b3eef1df34171c50941bbf895945c7a7d3f7d8ecb8` |
