---
type: source
title: "작업 - damage-pipeline-structure-review"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - "source"
  - "작업-기록"
  - "전투"
  - "피해"
summary: "피해 파이프라인을 결과 API·출처 명시·Hit 분리를 거쳐 ApplyDamage → Damage GE 컴포넌트의 정방향 흐름으로 재설계한 2026-09-23 기록과 2026-09-27 헤드리스·2026-09-28 사람 확인"
source_type: task-record
source_id: src-20f7b73699ee66fdc835
sha256: a0a17c15cf78e70a44241c5e308fb09dff4089a59fb4ca524019f093f986c188
authority: primary
independence_key: ".agents/workflow/tasks/damage-pipeline-structure-review.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/damage-pipeline-structure-review.md"
raw_copy: ".raw/captured/a0a17c15cf78e70a44241c5e308fb09dff4089a59fb4ca524019f093f986c188.md"
claim_ids:
  - clm-a0a17c15cf-c1
  - clm-a0a17c15cf-c2
  - clm-a0a17c15cf-c3
  - clm-a0a17c15cf-c4
key_claims:
  - "2026-09-23 사용자 판단으로 피해 파이프라인은 Hit Wrapper GE·FWxHitEffectContext·FWxDamageResult를 없애고, 네 인자 ApplyDamage → Damage GE(ExecCalc 방어 판정) → DamageReaction·PerfectGuard·HitStop·AdditionalEffects 컴포넌트 순서로 결과가 앞으로만 흐르게 재설계됐으며 ApplyDamage는 Damage GE 적용 여부만 반환한다."
  - "사용자 지시(\"반응이 끝난 뒤에 만들어야해요\")로 추가 효과 Spec은 피해 반응 뒤 _AdditionalEffects가 만들며, 0 피해에도 적용하고 퍼펙트 가드일 때만 생략한다."
  - "2026-09-27 헤드리스 임시 자동화 테스트로 일반 적중·일반 가드·퍼펙트 가드에는 히트스톱이 걸리고 완전 경감 가드·반올림 0 피해에는 걸리지 않으며, 퍼펙트 가드에 맞은 반사 가능 투사체는 쏜 적에게 되돌아가고 반사 불가 투사체는 파괴됨을 AI가 확인했다."
  - "woogle은 2026-09-28 히트스톱 체감과 피해 파이프라인 재설계 전체 코드 리뷰를 통과시켰고, 상태 전이 순서 정리와 가드 방향 등 기획 확인 항목은 기록에 대기로 남아 있다."
---

# 작업 - damage-pipeline-structure-review

- 원본: `.agents/workflow/tasks/damage-pipeline-structure-review.md`
- 원자료 사본: `.raw/captured/a0a17c15cf78e70a44241c5e308fb09dff4089a59fb4ca524019f093f986c188.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 개요

피해 파이프라인 구조 검토에서 시작해 여러 단계의 사용자 판단으로 구조를 바꾼 2026-09-23 작업 기록이다(기준 HEAD `47b7f8bd7`). 결과 API 도입, 요청 타입·출처 명시, 피해 행 1회 조회, Hit 함수 분리를 거쳐 최종적으로 Hit Wrapper GE와 전용 Context를 없앤 정방향 흐름에 이르렀고, 중간 형태 여러 개는 뒤의 사용자 지시로 대체됐다. 상태는 완료(체크리스트 5/5 통과)다.

## 사람의 판단 원문

> 사용자 2026-09-23: "네. 제안 구조: 결과가 앞으로만 흐르게로 합시다. 구조가 단순화되고 직관적이 되는 것을 저는 선호합니다."

> 사용자 2026-09-23: "OnImmunityBlockGameplayEffectDelegate 를 쓰는 방식으로 재구현합시다."

> 사용자 2026-09-23: "사망 확인 같은 경우는 각 Effect 내부에서 조회하는게 더 바람직"

> 사용자 2026-09-23: "네 B안 전체로 진행하세요.", "반응이 끝난 뒤에 만들어야해요."

> 사용자 2026-09-23: "1,2,3 정리하고 테스트 코드도 없애주세요.", "1, 2 적용해주세요"

## 최종 구조(기록 시점)

- 흐름: 무기·투사체·AreaDamage·Finisher의 네 인자 `UWxCombatLibrary::ApplyDamage`(출처·레벨 추론, 권위·적대 판정) → `MakeDamageSpec` → Damage GE와 ExecCalc(Damage.CanGuard와 대상 태그로 방어 판정, `Damage.Guarded`·`Damage.PerfectGuarded` 결과 태그) → `_DamageReaction`·`_PerfectGuard`·`_HitStop`·`_AdditionalEffects` 컴포넌트. 컴포넌트는 Damage GE 생성자에 추가한 순서대로 실행된다.
- 사망 확인은 Damage GE의 TargetTagRequirements(Ability.Death)로 옮겼다. 적대 판정을 CanApply 컴포넌트로 옮기는 시도는 엔진이 Immunity 쿼리를 CanApply보다 먼저 돌려 아군 공격에도 극한 회피가 나 되돌렸다.
- 회피는 Dodge 어빌리티가 활성 동안 Immunity 차단 델리게이트를 구독해 막힌 Spec이 `UWxEffect_Damage`인지로 극한 회피를 판정한다. `Event.DodgeSuccess`·`Damage.Attack` 태그와 `bEvaded`는 지웠다.
- 추가 효과는 입력 전용 `FWxDamageEffectContext`가 GE 클래스 목록만 들고, `_AdditionalEffects`가 반응 뒤 같은 Context·Damage 레벨로 Spec을 만든다. 0 피해에도 적용하고 퍼펙트 가드면 생략한다.
- `_HitStop`은 피해 > 0 또는 퍼펙트 가드일 때만 걸고(Hit Cue와 같은 조건), `_PerfectGuard`가 원인 투사체를 `Reflect`한다. 투사체는 호출 전 Owner가 바뀌지 않았으면 파괴한다.
- 엔진 `UAdditionalEffectsGameplayEffectComponent`로 대체하는 안은 행별 목록·퍼펙트 가드 생략이 안 되어 쓰지 않았다.

## 검증 범위

- 2026-09-23 단계마다 WxEditor 빌드와 `Wx.Combat.Damage.Result` GAS 자동화를 돌렸고, 이 테스트 파일은 사용자 지시로 삭제되어 이후 상시 자동 회귀는 없다.
- 2026-09-27 헤드리스 게임(LV_DevCombat, 템플릿 적의 `BP_Katana`를 원인으로 ApplyDamage): 일반 적중 HP 100→86·양쪽 히트스톱, 퍼펙트 가드 피해 0·반사 GP 14·히트스톱, 완전 경감 가드(기본 경감 배율을 0.5로 올려 합 1.0) 피해 0·히트스톱 없음, 일반 가드 HP 86→79·SP 100→93·히트스톱, 반올림 0(DEF 1e6) 히트스톱 없음. 투사체 되돌림은 Owner·Instigator가 플레이어로 바뀌고 쏜 적 HP 100→86, 반사 불가 투사체는 적중 직후 파괴. 기본 가드 경감은 0.5라 기본 설정에서는 완전 경감이 없다.
- 사람: woogle 2026-09-28 히트스톱 체감·코드 리뷰 통과. 2026-09-23 피니셔 피해 행 누락 수정은 사용자 플레이로 HP 감소를 확인했다.

## 남은 과제(기록 시점)

- 자원 반영(SP→HP→GP 모디파이어 순서)과 사망·그로기 상태 전이 순서를 한곳에서 정하는 일.
- 기획 확인 대기: 가드 방향(도입 시 `_DamageReaction`의 가드 취소 조건도 수정), 공격별 그로기 파워, 그로기 중 피해 증가, 플로터 표시 대상·위치, 일반 가드의 추가 효과 통과, 커스텀 Context 할당 경로.

## 관련 주제

- [[피해 파이프라인]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-23 사용자 판단으로 피해 파이프라인은 Hit Wrapper GE·FWxHitEffectContext·FWxDamageResult를 없애고, 네 인자 ApplyDamage → Damage GE(ExecCalc 방어 판정) → DamageReaction·PerfectGuard·HitStop·AdditionalEffects 컴포넌트 순서로 결과가 앞으로만 흐르게 재설계됐으며 ApplyDamage는 Damage GE 적용 여부만 반환한다. ^c1
- 사용자 지시("반응이 끝난 뒤에 만들어야해요")로 추가 효과 Spec은 피해 반응 뒤 _AdditionalEffects가 만들며, 0 피해에도 적용하고 퍼펙트 가드일 때만 생략한다. ^c2
- 2026-09-27 헤드리스 임시 자동화 테스트로 일반 적중·일반 가드·퍼펙트 가드에는 히트스톱이 걸리고 완전 경감 가드·반올림 0 피해에는 걸리지 않으며, 퍼펙트 가드에 맞은 반사 가능 투사체는 쏜 적에게 되돌아가고 반사 불가 투사체는 파괴됨을 AI가 확인했다. ^c3
- woogle은 2026-09-28 히트스톱 체감과 피해 파이프라인 재설계 전체 코드 리뷰를 통과시켰고, 상태 전이 순서 정리와 가드 방향 등 기획 확인 항목은 기록에 대기로 남아 있다. ^c4
