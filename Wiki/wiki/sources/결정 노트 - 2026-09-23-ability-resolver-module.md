---
type: source
title: "결정 노트 - 2026-09-23-ability-resolver-module"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "MVVM"
  - "모듈"
summary: "Ability ViewModel Resolver를 WxGame에서 WxUI로 옮기고 CoreRedirects로 기존 클래스 경로를 호환시킨 2026-09-23 정적 확인 기록"
source_type: decision-note
source_id: src-3e382221986e197c9098
sha256: 7ba0c6f3766e89e2b0c45d22cac919c9118480fa6dcd006ba0dc9fbeb3383911
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-ability-resolver-module.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-ability-resolver-module.md"
raw_copy: ".raw/captured/7ba0c6f3766e89e2b0c45d22cac919c9118480fa6dcd006ba0dc9fbeb3383911.md"
claim_ids:
  - clm-da458a7691-c1
  - clm-da458a7691-c2
  - clm-da458a7691-c3
  - clm-da458a7691-c4
key_claims:
  - "2026-09-23 작업 트리에서 WxViewModelResolver_Ability는 WxGame에서 WxUI 플러그인(WXUI_API)으로 옮겨졌다."
  - "Ability Resolver 이동 후 DefaultEngine.ini의 CoreRedirects가 /Script/WxGame.WxViewModelResolver_Ability를 /Script/WxUI.WxViewModelResolver_Ability로 연결한다."
  - "Ability Resolver는 위젯 소유 PC의 Pawn에서 ASC를 얻고 ASC가 없거나 AbilityTags가 비면 nullptr를 반환한다."
  - "Ability Resolver 이동 노트는 기존 WBP 로드와 표시 동작을 확인하지 않은 정적 조사다."
---

# 결정 노트 - 2026-09-23-ability-resolver-module

- 원본: `.wiki/raw/notes/2026-09-23-ability-resolver-module.md`
- 원자료 사본: `.raw/captured/7ba0c6f3766e89e2b0c45d22cac919c9118480fa6dcd006ba0dc9fbeb3383911.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-23-ability-resolver-module.md`(제목 "Ability Resolver의 WxUI 소유와 이전 경로 호환").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-23`, 태그 `wx, ui, static-review`.
- 성격: 2026-09-23 사용자 요청에 따른 **미커밋 작업 트리의 정적 조사(빌드·실행 검증 아님)**. 조사 시점 기준이라 현재 코드와 다를 수 있다.
- 사용자 요청 원문은 노트에 없다("사용자 요청에 따른"이라고만 적음).

## 구현 관찰 (정적)

- `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModelResolver_Ability.h`: `WXUI_API`로 내보내는 Resolver 선언과 `AbilityTags` 속성.
- `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_Ability.cpp`: 위젯을 소유한 PC의 Pawn에서 `IAbilitySystemInterface`로 ASC를 얻는다. ASC가 없거나 `AbilityTags`가 비면 `nullptr`, 그 밖에는 AbilitySystem VM의 슬롯 캐시를 쓴다.
- `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`: `GameplayAbilities`, `GameplayTags`, `ModelViewViewModel` 의존성이 이미 있어 추가 의존성이 필요 없었다.
- `Config/DefaultEngine.ini` CoreRedirects: `/Script/WxGame.WxViewModelResolver_Ability` → `/Script/WxUI.WxViewModelResolver_Ability`.
- 클래스 이동만 했고 동작은 바꾸지 않았다.

## 검증 범위

- 기존 WBP의 로드와 표시 동작은 이 정적 조사에서 확인하지 않았다.
- 빌드 결과는 당시 Workflow 작업 기록(`.agents/workflow/tasks/ability-resolver-to-wxui.md`)에서 관리한다고 적었다. 이 노트 자체는 빌드 통과를 기록하지 않는다.

## 관련 주제

- [[UI 표시 구조]]
- [[모듈 구조와 코드 정리]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-23 작업 트리에서 WxViewModelResolver_Ability는 WxGame에서 WxUI 플러그인(WXUI_API)으로 옮겨졌다. ^c1
- Ability Resolver 이동 후 DefaultEngine.ini의 CoreRedirects가 /Script/WxGame.WxViewModelResolver_Ability를 /Script/WxUI.WxViewModelResolver_Ability로 연결한다. ^c2
- Ability Resolver는 위젯 소유 PC의 Pawn에서 ASC를 얻고 ASC가 없거나 AbilityTags가 비면 nullptr를 반환한다. ^c3
- Ability Resolver 이동 노트는 기존 WBP 로드와 표시 동작을 확인하지 않은 정적 조사다. ^c4
