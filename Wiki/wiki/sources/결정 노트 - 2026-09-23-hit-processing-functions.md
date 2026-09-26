---
type: source
title: "결정 노트 - 2026-09-23-hit-processing-functions"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "피해"
  - "GAS"
summary: "UWxEffectComponent_Hit 안에서 방어 판정·Spec 준비·결과 기반 반응을 새 타입 없이 함수 단위로 분리한 구조 개선 기록."
source_type: decision-note
source_id: src-4f66ee9aaa510d2c47b8
sha256: 3941a4f557360b153dbcf8c37d015c04740e07cffca9cf9cb8e19ce77a7580d2
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-hit-processing-functions.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-hit-processing-functions.md"
raw_copy: ".raw/captured/3941a4f557360b153dbcf8c37d015c04740e07cffca9cf9cb8e19ce77a7580d2.md"
claim_ids:
  - clm-a21cb9ca5f-c1
  - clm-a21cb9ca5f-c2
  - clm-a21cb9ca5f-c3
key_claims:
  - "2026-09-23 작업 트리의 UWxEffectComponent_Hit는 방어 판정을 EvaluateDefense, Spec 준비를 PrepareDamageSpecs, 결과 기반 반응을 ProcessHitReactions로 분리했다."
  - "Hit 처리 함수 분리는 새 UObject·데이터 타입·파일 계층을 추가하지 않았고 사망/그로기 전이 시점을 바꾸지 않았다."
  - "Hit 처리 함수 분리 노트에는 빌드·실행 검증 결과가 없고 Workflow Task로 넘긴다."
---

# 결정 노트 - 2026-09-23-hit-processing-functions

- 원본: `.wiki/raw/notes/2026-09-23-hit-processing-functions.md`
- 원자료 사본: `.raw/captured/3941a4f557360b153dbcf8c37d015c04740e07cffca9cf9cb8e19ce77a7580d2.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-hit-processing-functions.md` (source: MANUAL, ingested: 2026-09-23).
- 2026-09-23 작업 트리의 `UWxEffectComponent_Hit`를 기준으로 한 기록이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.
- 사용자 요청은 다음 구조 개선의 자동 진행이며, 직전 검토의 불필요한 타입 추가를 줄이라는 방향을 유지한다고 노트가 적는다. 사용자 발화 원문은 노트에 인용되어 있지 않다.

## 구현 관찰(함수별 책임)

- `EvaluateDefense`: 무적/가드 가능/퍼펙트 가드/일반 가드 태그를 읽어 기존 `EWxDamageDefense`를 반환한다. 이벤트나 자원 변경은 하지 않는다.
- `PrepareDamageSpecs`: Linked Damage Spec과 추가 효과 Spec을 준비한다. 추가 효과의 출처 태그는 피해 적용과 반응 이벤트 전에 캡처한다.
- `ProcessHitReactions`: 확정된 `FWxDamageResult`를 읽어 Hit Cue, 일반 피격/가해 이벤트, 퍼펙트 가드 반응을 처리한다.
- `OnGameplayEffectApplied`: Context 검사 → 회피 조기 종료 → 피해 적용/실패 종료 → 결과 보존 → 반응 → 추가 효과 적용 순서를 유지한다.

## 범위 제한(바꾸지 않은 것)

- 새 UObject·데이터 타입·파일 계층을 추가하지 않았다.
- DamageResponse의 결과 수집과 플로터, 자원 반영 중 사망/그로기 전이 시점은 변경하지 않았다.

## 검증 범위

- 근거 파일: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, 같은 이름의 헤더, GAS 회귀 테스트 `Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp`.
- 이 노트 자체에는 빌드·실행 결과가 없고 "실행 검증과 제한은 Workflow Task에 기록한다"고만 적는다.

## 관련 주제

- [[피해 파이프라인]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-23 작업 트리의 UWxEffectComponent_Hit는 방어 판정을 EvaluateDefense, Spec 준비를 PrepareDamageSpecs, 결과 기반 반응을 ProcessHitReactions로 분리했다. ^c1
- Hit 처리 함수 분리는 새 UObject·데이터 타입·파일 계층을 추가하지 않았고 사망/그로기 전이 시점을 바꾸지 않았다. ^c2
- Hit 처리 함수 분리 노트에는 빌드·실행 검증 결과가 없고 Workflow Task로 넘긴다. ^c3
