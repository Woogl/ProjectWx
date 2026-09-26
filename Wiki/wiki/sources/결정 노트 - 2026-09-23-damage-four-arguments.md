---
type: source
title: "결정 노트 - 2026-09-23-damage-four-arguments"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "피해"
  - "API"
summary: "사용자 승인으로 FWxDamageRequest를 없애고 ApplyDamage를 Causer·Target·피해 행·HitResult 네 인자로 되돌려 출처·레벨 추론을 복원한 기록"
source_type: decision-note
source_id: src-9261f9040fc8cc2e29a0
sha256: 0d84b879c8359394ddfd36127f7ad1ba2532f6c3ef7a182342af167f671a6c77
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-damage-four-arguments.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-damage-four-arguments.md"
raw_copy: ".raw/captured/0d84b879c8359394ddfd36127f7ad1ba2532f6c3ef7a182342af167f671a6c77.md"
claim_ids:
  - clm-109b37c644-c1
  - clm-109b37c644-c2
  - clm-109b37c644-c3
key_claims:
  - "2026-09-23 사용자 승인으로 FWxDamageRequest가 제거되고 ApplyDamage는 Causer·Target·DamageTableRow·HitResult 네 인자를 받는 유일한 진입점이 됐다."
  - "네 인자 ApplyDamage는 Causer의 ASC를 먼저 찾고 없으면 직접 Owner의 ASC를 사용한다."
  - "네 인자 ApplyDamage에서 투사체는 저장된 ProjectileLevel과 Ability=nullptr를, 일반 공격은 적용 직전 AnimatingAbility와 그 레벨(없으면 1)을 사용한다."
---

# 결정 노트 - 2026-09-23-damage-four-arguments

- 원본: `.wiki/raw/notes/2026-09-23-damage-four-arguments.md`
- 원자료 사본: `.raw/captured/0d84b879c8359394ddfd36127f7ad1ba2532f6c3ef7a182342af167f671a6c77.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-23-damage-four-arguments.md`(제목 "ApplyDamage 네 인자 인터페이스 복원").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-23`, 태그 `wx, damage`.
- 성격: 사용자 승인에 따른 API 복원과 구현 관찰. 실행 검증은 당시 Workflow Task에서 관리한다고 적었고 이 노트에는 없다.
- 조사 시점 기준이다. 같은 날 이후 [[결정 노트 - 2026-09-23-damage-forward-flow]]에서 반환형이 bool로 바뀌고 `FWxDamageResult`가 삭제되어 현재 코드와 다르다.

## 사람의 판단 (노트의 간접 기록)

> 노트 2026-09-23: "사용자가 이전 인자의 편의성을 선호했고 출처·Ability·레벨 추론을 복원하는 변경을 승인했다."

## 변경 내용 (구현 관찰)

- 유일한 진입점: `FWxDamageResult ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult)`. 반환값은 평탄화한 결과를 유지했다.
- `FWxDamageRequest`를 제거하고 네이티브 호출부 네 곳의 요청 조립을 없앴다. 별도 래퍼나 오버로드는 추가하지 않는다.
- ASC 선택: Causer의 ASC를 먼저 찾고, 없으면 직접 Owner의 ASC를 쓴다. Source·Target ASC와 출처 권위 검사는 유지.
- 레벨·Ability 추론: 투사체는 저장된 `ProjectileLevel`과 `Ability=nullptr`. 반사로 Owner가 바뀌면 현재 Owner의 ASC를 쓰고 발사 레벨은 유지한다. 일반 공격은 적용 직전 `AnimatingAbility`와 그 레벨, 없으면 레벨 1.
- 테스트의 명시 요청 시나리오를 실제 템플릿 투사체의 Owner/저장 레벨/Owner 교체 검사로 바꿨다. 피니셔·무기·범위 공격의 계산과 이벤트 순서는 유지했다.

## 검증 범위

- 요청 구조체 API를 쓰는 외부 Blueprint가 있다면 네 인자 노드로 바꿔야 한다. 최초 Content 검색에서는 ApplyDamage 참조가 없었다(문자열 검색이며 Blueprint 실행 검증 아님).
- 근거 파일: `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp`.

## 관련 주제

- [[피해 파이프라인]]

## 핵심 주장

- 2026-09-23 사용자 승인으로 FWxDamageRequest가 제거되고 ApplyDamage는 Causer·Target·DamageTableRow·HitResult 네 인자를 받는 유일한 진입점이 됐다. ^c1
- 네 인자 ApplyDamage는 Causer의 ASC를 먼저 찾고 없으면 직접 Owner의 ASC를 사용한다. ^c2
- 네 인자 ApplyDamage에서 투사체는 저장된 ProjectileLevel과 Ability=nullptr를, 일반 공격은 적용 직전 AnimatingAbility와 그 레벨(없으면 1)을 사용한다. ^c3
