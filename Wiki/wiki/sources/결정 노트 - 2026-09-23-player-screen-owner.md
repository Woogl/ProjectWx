---
type: source
title: "결정 노트 - 2026-09-23-player-screen-owner"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "구조"
summary: "사망·대화 화면 클래스와 태그 관찰을 UIManager 서브시스템·전역 설정에서 컨트롤러 BP의 UWxPlayerLayoutComponent로 옮긴 결정과 정적 조사 기록."
source_type: decision-note
source_id: src-e5701065b4da6e6690fe
sha256: e318190094a7cb664fcbda9082af4a806045782bd2c319f40b3b616352c58175
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-player-screen-owner.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-player-screen-owner.md"
raw_copy: ".raw/captured/e318190094a7cb664fcbda9082af4a806045782bd2c319f40b3b616352c58175.md"
claim_ids:
  - clm-88d74f5492-c1
  - clm-88d74f5492-c2
  - clm-88d74f5492-c3
key_claims:
  - "사용자 확정 결정에 따라 UWxUIDeveloperSettings에는 LayoutClass·ConfirmationPopupClass만 남고 사망·대화 화면 클래스는 UWxPlayerLayoutComponent로 옮겨졌다."
  - "UWxPlayerLayoutComponent는 폰 교체 시 대화 창은 닫지만 사망 화면은 닫지 않는다."
  - "노트는 사용자가 2026-09-23 사망·부활·대화 동작을 인게임에서 확인했다고 기록한다."
---

# 결정 노트 - 2026-09-23-player-screen-owner

- 원본: `.wiki/raw/notes/2026-09-23-player-screen-owner.md`
- 원자료 사본: `.raw/captured/e318190094a7cb664fcbda9082af4a806045782bd2c319f40b3b616352c58175.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-player-screen-owner.md` (source: MANUAL, ingested: 2026-09-23, revision: `47b7f8bd7`).
- 2026-09-23 HEAD `47b7f8bd7` 및 미커밋 작업 트리의 **정적 조사(빌드·실행 검증 아님)**와 사용자 결정 기록이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 확정 결정(사용자 확정으로 기록됨)

> 노트 결정 원문: "`UWxUIDeveloperSettings`에는 UI의 틀(`LayoutClass`, `ConfirmationPopupClass`)만 둔다. 게임플레이에 반응하는 화면은 컨트롤러 BP의 `UWxPlayerLayoutComponent`에 둔다."

- 근거: 사망·대화 화면은 `a8cffba6c`(2026-07-28)에서 HUD와 함께 설정으로 옮겨졌고, 이후 HUD만 `f8718e3a6`(2026-08-23)에서 컴포넌트로 돌아가 이 둘이 설정에 남아 있었다.
- 효과: 모드마다 컨트롤러 BP를 달리하면 다른 화면을 쓸 수 있다. 서브시스템은 레이아웃·팝업·일시정지만 맡는다.
- 대가: 값이 ini에서 BP 에셋으로 옮겨져 git diff로 리뷰할 수 없다.

## 구현 관찰(확인한 계약)

- 컴포넌트는 폰이 바뀔 때(`OldPawn != NewPawn`) 새 폰의 `Ability.Death`·`State.Dialogue` 태그 관찰로 갈아타고 `EndPlay`에서 관찰을 끊는다.
- 관찰을 갈아탈 때 대화 창은 닫지만 사망 화면은 닫지 않는다. 부활이 폰을 교체하며, 사망 화면은 부활 요청이 완료될 때 스스로 비활성화된다.
- 빈 클래스 판정은 `UWxAsyncAction_PushWidgetToLayer`가 하며 값이 비면 화면을 띄우지 않는다.
- `UWxUIManagerSubsystem`의 `TrackedPlayerController`는 일시정지에만 쓰이고 빙의를 구독하지 않는다.
- 값: `BP_PlayerController`에 `WBP_DeathScreen`·`WBP_DialogueScreen`, `BP_FrontEndPlayerController`는 비어 있다.

## 검증 범위

- 노트는 사용자가 사망·부활·대화 동작을 인게임에서 확인했다고 기록한다(2026-09-23). 사용자 발화 원문은 노트에 없다.
- 빌드 근거는 `.agents/workflow/tasks/player-screen-classes-to-layout-component.md`에 있다고 적는다.

## 관련 주제

- [[UI 표시 구조]]
- [[체크포인트와 리스폰]]
- [[작업 - player-screen-classes-to-layout-component]]

## 핵심 주장

- 사용자 확정 결정에 따라 UWxUIDeveloperSettings에는 LayoutClass·ConfirmationPopupClass만 남고 사망·대화 화면 클래스는 UWxPlayerLayoutComponent로 옮겨졌다. ^c1
- UWxPlayerLayoutComponent는 폰 교체 시 대화 창은 닫지만 사망 화면은 닫지 않는다. ^c2
- 노트는 사용자가 2026-09-23 사망·부활·대화 동작을 인게임에서 확인했다고 기록한다. ^c3
