---
type: source
title: "결정 노트 - 2026-09-23-damage-context-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "피해"
  - "GAS"
summary: "Damage EffectContext에서 소비자가 없는 테이블 참조 저장·복제를 없애고 중복 피해 수치를 FWxDamageResult로 합친 2026-09-23 사용처 조사와 변경 기록"
source_type: decision-note
source_id: src-e1d67fb4d45dcb614edc
sha256: 043c6ec9f9ee4795580b7262c786ba60fd3d28ce48e7352cc8ade80748120739
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-damage-context-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-damage-context-cleanup.md"
raw_copy: ".raw/captured/043c6ec9f9ee4795580b7262c786ba60fd3d28ce48e7352cc8ade80748120739.md"
claim_ids:
  - clm-8232dca0b4-c1
  - clm-8232dca0b4-c2
  - clm-8232dca0b4-c3
key_claims:
  - "2026-09-23 소스 검색에서 Damage Context의 DamageTable·DamageRowName은 생성과 NetSerialize 외에 읽는 곳이 없어 제거됐다."
  - "2026-09-23 정리 후 Damage Context의 네트워크 형식은 기본 GameplayEffectContext와 방어 2비트만 남아 이전 형식과 비트 호환되지 않는다."
  - "Damage Context 정리의 회귀 테스트는 직렬화 왕복 검사이며 실제 네트워크 세션 검증과 구분된다."
---

# 결정 노트 - 2026-09-23-damage-context-cleanup

- 원본: `.wiki/raw/notes/2026-09-23-damage-context-cleanup.md`
- 원자료 사본: `.raw/captured/043c6ec9f9ee4795580b7262c786ba60fd3d28ce48e7352cc8ade80748120739.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-23-damage-context-cleanup.md`(제목 "Damage Context의 미사용 데이터와 중복 수치 제거").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-23`, 태그 `wx, damage`.
- 성격: 사용자의 불필요한 구조 제거 요청에 따라 2026-09-23 작업 트리에서 사용처를 확인하고 정리한 기록. 빌드·실행 결과는 당시 Workflow Task에 기록한다고 적었고 이 노트에는 없다.
- 조사 시점 기준이다. 같은 날 이후 [[결정 노트 - 2026-09-23-damage-forward-flow]]에서 `FWxHitEffectContext`와 `FWxDamageResult`가 삭제되어 현재 코드와 다르다.

## 사용자 요청 (노트의 간접 기록)

> 노트 2026-09-23: "사용자의 불필요한 구조 제거 요청에 따라 2026-09-23 작업 트리에서 사용처를 확인했다."

## 변경 내용 (구현 관찰)

- **테이블 참조 제거**: Source/Plugins 검색상 `DamageTable`/`DamageRowName`은 Context 생성과 `NetSerialize` 외에 읽는 곳이 없었다. 피해 행 조회는 이미 `ApplyDamageRequest`에서 끝나므로 두 필드, 생성자 행 인자, 테이블 객체 매핑, 행 이름 직렬화를 제거했다.
- **중복 수치 제거**: `DamageMagnitude`/`ReflectMagnitude`/`bHasReflect`가 Context와 `FWxDamageResult`에 중복됐다. DamageResponse는 Result에 수치를 기록하고 Hit는 후속 이벤트 전에 이를 지역 결과로 보존한다.
- **유지한 것**: 방어 플래그(실행 계산·네트워크용, 결과와 수명이 다름), 추가 효과 목록, 결과 태그·대상 태그(실제 소비가 있음).
- **네트워크 형식**: 기본 GameplayEffectContext + 방어 2비트만 남는다. 이전 형식과 비트 호환이 아니므로 같은 빌드의 서버·클라이언트를 써야 한다.

## 검증 범위

- 기존 GAS 회귀에 기본 Context 원점/방어 비트 왕복, 수신 시 로컬 수치·추가 효과 초기화 검사를 추가했다. 노트는 이를 **실제 네트워크 세션 검증과 구분**한다.
- 근거 파일: `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp`, `.../AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `.../AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `.../Tests/WxDamageResultTest.cpp`.

## 관련 주제

- [[피해 파이프라인]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-23 소스 검색에서 Damage Context의 DamageTable·DamageRowName은 생성과 NetSerialize 외에 읽는 곳이 없어 제거됐다. ^c1
- 2026-09-23 정리 후 Damage Context의 네트워크 형식은 기본 GameplayEffectContext와 방어 2비트만 남아 이전 형식과 비트 호환되지 않는다. ^c2
- Damage Context 정리의 회귀 테스트는 직렬화 왕복 검사이며 실제 네트워크 세션 검증과 구분된다. ^c3
