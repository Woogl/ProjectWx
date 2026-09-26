---
type: source
title: "작업 - player-screen-classes-to-layout-component"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "플레이어"
summary: "사망·대화 화면 클래스를 UI 개발자 설정에서 플레이어 레이아웃 컴포넌트로 옮기고 태그 관찰 책임도 함께 이동한 완료 작업 기록"
source_type: task-record
source_id: src-0a24198e3cfec8246327
sha256: 497e41550f7115470f33ede2b94572b0960d4406dd313213b06421dc8f7d7f4c
authority: primary
independence_key: ".agents/workflow/tasks/player-screen-classes-to-layout-component.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/player-screen-classes-to-layout-component.md"
raw_copy: ".raw/captured/497e41550f7115470f33ede2b94572b0960d4406dd313213b06421dc8f7d7f4c.md"
claim_ids:
  - clm-75f119c715-c1
  - clm-75f119c715-c2
  - clm-75f119c715-c3
key_claims:
  - "사망·대화 화면 클래스는 UWxUIDeveloperSettings에서 제거되어 UWxPlayerLayoutComponent의 DeathScreenClass·DialogueScreenClass로 옮겨졌다."
  - "UWxPlayerLayoutComponent는 폰 교체 시 대화 창만 닫고 사망 화면은 닫지 않으며, 사망 화면은 부활 요청 완료 때 스스로 비활성화된다."
  - "화면 클래스 이동 작업은 2026-09-23 사용자가 사망·부활·대화 흐름을 인게임에서 확인했으나 별도 코드 리뷰 승인 기록은 없다."
---

# 작업 - player-screen-classes-to-layout-component

- 원본: `.agents/workflow/tasks/player-screen-classes-to-layout-component.md`
- 원자료 사본: `.raw/captured/497e41550f7115470f33ede2b94572b0960d4406dd313213b06421dc8f7d7f4c.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

`UWxUIDeveloperSettings`에 있던 `DeathScreenClass`·`DialogueScreenClass`를 컨트롤러 BP의 `UWxPlayerLayoutComponent`로 옮긴 작업이다. 상태는 완료(2026-09-23)이며 사용자가 인게임에서 확인했다.

## 확정 설계 (2026-09-23 사용자 확정)

- 설정에는 UI의 틀(`LayoutClass`, `ConfirmationPopupClass`)만 두고 게임 화면은 컨트롤러 BP 쪽에 둔다.
  - 근거: HUD는 이미 `UWxPlayerLayoutComponent::LayoutClass`, 메뉴는 `UWxHUDLayout`에 있다. 사망·대화 화면은 `a8cffba6c`(07-28)에서 HUD와 함께 설정으로 옮겨졌다가 HUD만 `f8718e3a6`(08-23)에서 컴포넌트로 돌아가고 남아 있었다.
- 태그 관찰(`Ability.Death`·`State.Dialogue`)과 대화 창 수명을 `UWxUIManagerSubsystem`에서 `UWxPlayerLayoutComponent`로 옮긴다. 서브시스템은 레이아웃·팝업·일시정지만 맡고 `TrackedPlayerController`는 일시정지용으로 남긴다.
- 폰이 바뀔 때 대화 창만 닫고 사망 화면은 닫지 않는다. 부활이 폰을 교체하며, 사망 화면은 부활 요청 완료 때 스스로 비활성화된다.
- 대가: 값이 ini에서 BP 에셋으로 옮겨져 git diff로 리뷰할 수 없고, 모드마다 다른 화면을 쓰려면 컨트롤러 BP 자식이 필요하다.

## 구현

- `WxPlayerLayoutComponent.h/.cpp`: `DeathScreenClass`·`DialogueScreenClass`(EditDefaultsOnly) 추가, `WatchPawnTags`·태그 핸들러·대화 창 push 완료·닫기 처리를 이동. 폰 교체 때 관찰 대상을 바꾸고 `EndPlay`에서 관찰을 끊는다.
- `WxUIManagerSubsystem.h/.cpp`: 빙의 구독·태그 관찰·대화 관련 멤버 제거.
- `WxUIDeveloperSettings.h`, `DefaultGame.ini`: 두 필드와 값 제거.
- 사용자가 `BP_PlayerController` → `PlayerLayoutComponent`에 `WBP_DeathScreen`·`WBP_DialogueScreen`을 입력했다. `BP_FrontEndPlayerController`는 비워 두었다.
- 커밋 `58f01692c`(이번 변경만 선별), 사용자가 푸시.

## 검증 범위

- AI 빌드: 첫 build-doctor는 다른 세션(wx-09)의 VM 변경 때문에 UHT 단계에서 실패, 재실행에서 WxEditor Win64 Development 성공·경고 0(다른 세션의 미커밋 변경 포함).
- 사람 인게임(2026-09-23): 사망 화면 표시·부활 시 닫힘, 대화 창 표시·종료 후 HUD 복귀, 부활 뒤 재표시를 확인했다.

> 사용자 2026-09-23: "잘 됩니다"

- 인간 코드 리뷰의 별도 승인 기록은 없다.

## 관련 주제

- [[UI 표시 구조]]
- [[체크포인트와 리스폰]]
- [[결정 노트 - 2026-09-23-player-screen-owner]]

## 핵심 주장

- 사망·대화 화면 클래스는 UWxUIDeveloperSettings에서 제거되어 UWxPlayerLayoutComponent의 DeathScreenClass·DialogueScreenClass로 옮겨졌다. ^c1
- UWxPlayerLayoutComponent는 폰 교체 시 대화 창만 닫고 사망 화면은 닫지 않으며, 사망 화면은 부활 요청 완료 때 스스로 비활성화된다. ^c2
- 화면 클래스 이동 작업은 2026-09-23 사용자가 사망·부활·대화 흐름을 인게임에서 확인했으나 별도 코드 리뷰 승인 기록은 없다. ^c3
