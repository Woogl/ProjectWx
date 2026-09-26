---
type: source
title: "결정 노트 - 2026-09-23-interaction-list-vm"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "상호작용"
  - "UI"
  - "MVVM"
summary: "상호작용 스캐너 신호를 OnRowsChanged 하나로 합쳐 목록 VM이 행 VM을 전부 재생성하게 한 구조와 문구 출처 원칙에 대한 사용자 결정·정적 조사 기록."
source_type: decision-note
source_id: src-281f68be3034243b724f
sha256: dce4c862ecb2c25d2de83da215c577e4d4b5a10a8229579b1baf7e860869d337
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-interaction-list-vm.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-interaction-list-vm.md"
raw_copy: ".raw/captured/dce4c862ecb2c25d2de83da215c577e4d4b5a10a8229579b1baf7e860869d337.md"
claim_ids:
  - clm-5a5eb0be16-c1
  - clm-5a5eb0be16-c2
  - clm-5a5eb0be16-c3
  - clm-5a5eb0be16-c4
key_claims:
  - "사용자는 상호작용 목록 VM과 행 VM의 두 클래스 구조와 리졸버를 유지하기로 확정했다."
  - "2026-09-23 구현에서 상호작용 스캐너 신호는 OnRowsChanged 하나로 합쳐졌고 목록 VM은 신호마다 행 VM 전체를 다시 만든다."
  - "상호작용 문구는 실제 행동의 주인(장치 StateTree, 대화 컴포넌트, 아이템 데이터, 피니시 어빌리티)이 갖고 모든 문구를 StateTree로 옮기는 안은 기각되었다."
  - "상호작용 목록 VM 변경은 이 노트 시점에 빌드·WBP 컴파일만 통과했고 인게임 동작은 미검증이었다."
---

# 결정 노트 - 2026-09-23-interaction-list-vm

- 원본: `.wiki/raw/notes/2026-09-23-interaction-list-vm.md`
- 원자료 사본: `.raw/captured/dce4c862ecb2c25d2de83da215c577e4d4b5a10a8229579b1baf7e860869d337.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-interaction-list-vm.md` (source: MANUAL, ingested: 2026-09-23, revision: `7d2a20408`).
- 2026-09-23 HEAD `7d2a20408` 기준 **정적 조사(빌드·실행 검증 아님)**와 사용자 결정 기록이다. 구현 커밋은 `f98eef471`(목록 VM), `9556bfc78`(픽업 문구). 노트 날짜 기준이라 현재 코드와 다를 수 있다(예: 이후 [[결정 노트 - 2026-09-24-interaction-contract-options-only]]가 상호작용 계약을 바꿨다).

## 확정 결정(사용자 확정으로 기록됨)

노트의 결정 문장 원문 중 판단에 해당하는 부분:

> "목록 VM(`UWxViewModel_InteractionList`)과 행 VM(`UWxViewModel_Interaction`)의 두 클래스 구조를 유지한다."
> "겹친 대상을 목록으로 보여 주고 휠로 선택하는 동작은 유지한다."
> "리졸버는 유지한다."
> "스캐너 신호를 `OnRowsChanged` 하나로 합친다. 목록 VM은 신호마다 스캐너에서 목록과 선택을 읽어 행 전체를 다시 만든다."
> "문구는 상호작용했을 때 실제로 일어날 행동의 주인이 갖는다. 모든 문구를 StateTree로 옮기는 안은 기각했다."
> "픽업 문구의 `[F]` 키 표기를 제거했다."

- 행 VM 하나로 합치는 안은 한 클래스가 두 역할을 맡고 모듈 이동과 CoreRedirects 3개가 필요해 기각했다.
- 리졸버 유지 이유: 엔진 Create Instance는 `NewObject`만 호출하고 `DestroyInstance`는 리졸버에만 불린다(UE 5.8 `MVVMViewClass.cpp`).
- 문구 주인: 장치는 StateTree, 대화 액터는 대화 컴포넌트, 픽업은 아이템 데이터(실행 중 스폰되는 드랍), 피니시는 피니시 어빌리티. 키 표시는 `WBP_Interaction`의 `CommonActionWidget`(`DT_InputActions`의 `Interact` 행)이 맡는다.
- 대가: 선택을 바꿀 때마다 ListView 엔트리가 새로 붙는다. 현재 `bSelected`로 이미지 가시성만 바꾸므로 화면 차이는 없지만, 선택 전환 애니메이션을 넣으려면 선택 신호를 다시 나눠야 한다.
- 스캐너 늦은 도착 관찰(`OnAnyScannerReady`)은 스캐너가 `AWxPlayerController` 생성자 컴포넌트로 돌아와(`97eb35f97`) 제거했다.

## 구현 관찰(확인한 계약)

- 스캐너 `UWxInteractionScannerComponent`는 기존 대상 순서를 보존하고 새 후보를 거리순으로 뒤에 붙인다. 행은 대상의 선택지 하나당 하나이며 문구·값은 스캔마다 다시 읽는다.
- 대상·선택지 값·문구가 달라졌을 때만 행을 교체하고 `OnRowsChanged`를 발행한다. 선택 변경(`CycleSelection`)도 같은 신호로 발행하며, 외곽선은 선택을 소유한 스캐너만 건다.
- 목록 VM은 `Entries`만 FieldNotify로 노출하고 입력은 `RequestInteract`·`RequestCycle`로 스캐너에 넘긴다. 스캐너(WxWorld)를 직접 구독하므로 WxGame에 둔다.
- 리졸버는 위젯 소유 PC를 Outer로 목록 VM을 만들고 `FindComponentByClass`로 스캐너를 넘긴다. 헤더 주석: "나중에 주입하는 구조로 바꾸면 늦은 도착 처리가 다시 필요하다".
- 행 VM은 WxUI에 있고 `Prompt`·`bSelected`만 가지는 불변 VM이다(setter·cpp 없음).

## 검증 범위

- Task 기록 기준 Development 빌드와 `WBP_InteractionList`·`WBP_Interaction`·`WBP_GameLayout` 컴파일(오류 0, 경고 0) 성공.
- 인게임은 실행하지 않았다. 목록 표시·휠 선택·외곽선·선택지 실행·다중 선택지 장치·범위 이탈·리스폰 후 동작은 인간 확인 대상으로 남았다(이후 인게임 확인은 [[결정 노트 - 2026-09-26-interaction-list-play-acceptance]] 참고).

## 관련 주제

- [[상호작용과 장치]]
- [[UI 표시 구조]]
- [[작업 - interaction-list-vm-simplification]]

## 핵심 주장

- 사용자는 상호작용 목록 VM과 행 VM의 두 클래스 구조와 리졸버를 유지하기로 확정했다. ^c1
- 2026-09-23 구현에서 상호작용 스캐너 신호는 OnRowsChanged 하나로 합쳐졌고 목록 VM은 신호마다 행 VM 전체를 다시 만든다. ^c2
- 상호작용 문구는 실제 행동의 주인(장치 StateTree, 대화 컴포넌트, 아이템 데이터, 피니시 어빌리티)이 갖고 모든 문구를 StateTree로 옮기는 안은 기각되었다. ^c3
- 상호작용 목록 VM 변경은 이 노트 시점에 빌드·WBP 컴파일만 통과했고 인게임 동작은 미검증이었다. ^c4
