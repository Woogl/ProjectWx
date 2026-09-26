---
type: source
title: "결정 노트 - 2026-09-23-apply-damage-unification"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "피해"
  - "API"
summary: "bool ApplyDamage와 ApplyDamageWithResult를 FWxDamageResult 반환 ApplyDamage 하나로 합친 2026-09-23 사용자 요청과 호출처 검색 기록"
source_type: decision-note
source_id: src-b8eecc38d8d7eccd1975
sha256: 929c5313575c83b7afad99e78d7a113cea8c828666b72081d87140a850f78d70
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-apply-damage-unification.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-apply-damage-unification.md"
raw_copy: ".raw/captured/929c5313575c83b7afad99e78d7a113cea8c828666b72081d87140a850f78d70.md"
claim_ids:
  - clm-48f0ba3f04-c1
  - clm-48f0ba3f04-c2
  - clm-48f0ba3f04-c3
key_claims:
  - "2026-09-23 사용자 요청으로 UWxCombatLibrary의 bool ApplyDamage와 ApplyDamageWithResult가 FWxDamageResult를 반환하는 ApplyDamage 하나로 통합됐다."
  - "ApplyDamage 반환 API 통합은 출처 추론과 피해 처리 정책을 바꾸지 않고 반환 API만 합쳤다."
  - "ApplyDamage 통합 당시 C++ 검색에서 두 기존 함수의 호출자는 자동화 테스트뿐이었고 Content uasset 문자열 검색에서도 참조가 없었다."
---

# 결정 노트 - 2026-09-23-apply-damage-unification

- 원본: `.wiki/raw/notes/2026-09-23-apply-damage-unification.md`
- 원자료 사본: `.raw/captured/929c5313575c83b7afad99e78d7a113cea8c828666b72081d87140a850f78d70.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-23-apply-damage-unification.md`(제목 "ApplyDamage 반환 API 통합").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-23`, 태그 `wx, damage`.
- 성격: 사용자 요청에 따른 API 통합과 사용처 검색. 빌드·실행 결과는 당시 Workflow Task에 기록한다고 적고 이 노트에는 없다.
- 조사 시점 기준이다. 같은 날 이후 노트([[결정 노트 - 2026-09-23-damage-forward-flow]])에서 `FWxDamageResult`가 삭제되고 `ApplyDamage` 반환형이 다시 바뀌었으므로 현재 코드와 다르다.

## 사용자 요청 (노트의 간접 기록)

> 노트 2026-09-23: "사용자가 두 함수의 통합을 요청했다."

## 변경 내용 (구현 관찰)

- `UWxCombatLibrary::ApplyDamage`가 `FWxDamageResult`를 반환하도록 바꾸고 `ApplyDamageWithResult`와 bool 래퍼를 제거했다. 성공 여부만 필요한 호출은 `.bApplied`를 읽는다.
- 출처를 직접 지정하는 C++ `ApplyDamageRequest`는 유지했다.
- 출처 추론·피해 처리 정책은 그대로이고 반환 API만 합쳤다. 기존 GAS 테스트를 통합 함수로 옮겼다.

## 사용처 검색 (정적)

- Source/Plugins C++ 검색에서 두 기존 함수의 호출자는 자동화 테스트뿐이었다.
- Content uasset 문자열 검색에서 두 함수 이름 참조는 없었다.
- 외부 Blueprint 사용처가 있다면 bool 반환 핀은 결과의 `bApplied`로, `ApplyDamageWithResult` 노드는 `ApplyDamage`로 바꿔야 한다. 노트는 이 검색을 전체 Blueprint 실행 검증으로 확대하지 않는다고 밝힌다.
- 근거 파일: `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageCompatibility.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp`.

## 관련 주제

- [[피해 파이프라인]]
- [[모듈 구조와 코드 정리]]

## 핵심 주장

- 2026-09-23 사용자 요청으로 UWxCombatLibrary의 bool ApplyDamage와 ApplyDamageWithResult가 FWxDamageResult를 반환하는 ApplyDamage 하나로 통합됐다. ^c1
- ApplyDamage 반환 API 통합은 출처 추론과 피해 처리 정책을 바꾸지 않고 반환 API만 합쳤다. ^c2
- ApplyDamage 통합 당시 C++ 검색에서 두 기존 함수의 호출자는 자동화 테스트뿐이었고 Content uasset 문자열 검색에서도 참조가 없었다. ^c3
