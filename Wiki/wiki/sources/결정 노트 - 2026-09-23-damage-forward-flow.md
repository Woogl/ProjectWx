---
type: source
title: "결정 노트 - 2026-09-23-damage-forward-flow"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "피해"
  - "GAS"
summary: "Hit Wrapper GE와 전용 EffectContext를 없애고 ApplyDamage 판정에서 Damage GE 컴포넌트 반응으로 결과가 앞으로만 흐르게 한 2026-09-23 결정들"
source_type: decision-note
source_id: src-394a6c2707ea6f85c043
sha256: c66645cc2cd732a62dd49fcdb4babb939a7d1b1cf5b24ccf3fd83ffdeb67a206
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-damage-forward-flow.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-damage-forward-flow.md"
raw_copy: ".raw/captured/c66645cc2cd732a62dd49fcdb4babb939a7d1b1cf5b24ccf3fd83ffdeb67a206.md"
claim_ids:
  - clm-23cbef3fd0-c1
  - clm-23cbef3fd0-c2
  - clm-23cbef3fd0-c3
  - clm-23cbef3fd0-c4
key_claims:
  - "2026-09-23 사용자 승인으로 Hit Wrapper GE와 FWxHitEffectContext를 삭제하고 ApplyDamage 판정 → Damage GE → 반응 컴포넌트로 결과가 앞으로만 흐르게 재설계했다."
  - "2026-09-23 사용자 지시로 회피는 Dodge 어빌리티가 ASC의 OnImmunityBlockGameplayEffectDelegate를 구독해 막힌 Damage GE를 감지하는 방식으로 재구현됐다."
  - "2026-09-23 최종 상태에서 ApplyDamage는 FWxDamageResult 대신 Damage GE 적용 여부 bool만 반환한다."
  - "2026-09-23 사용자 지시로 추가 효과 Spec은 피해 반응이 끝난 뒤 _AdditionalEffects 컴포넌트가 실행 시점에 만든다."
---

# 결정 노트 - 2026-09-23-damage-forward-flow

- 원본: `.wiki/raw/notes/2026-09-23-damage-forward-flow.md`
- 원자료 사본: `.raw/captured/c66645cc2cd732a62dd49fcdb4babb939a7d1b1cf5b24ccf3fd83ffdeb67a206.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-23-damage-forward-flow.md`(제목 "Damage 결과를 앞으로만 흘리는 구조").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-23`, 태그 `wx, damage`.
- 성격: 2026-09-23 하루 동안 이어진 **사용자 결정·지시**와 그에 따른 구현 기록. 본문 뒤 "추가" 절 6개가 순서대로 앞 결정을 고쳐 나간다. 빌드·회귀 결과는 당시 Workflow Task에 기록한다고 적었고 이 노트에는 없다.
- 앞선 [[결정 노트 - 2026-09-23-damage-four-arguments]]의 네 인자 복원 뒤에 나온 재설계다. 조사 시점 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-23 (재설계안 승인): "구조가 단순화되고 직관적이 되는 것을 선호"

> 사용자 2026-09-23 (회피 재구현): "OnImmunityBlockGameplayEffectDelegate 를 쓰는 방식으로 재구현합시다."

> 사용자 2026-09-23 (Damage Effect 분할 제안에 대해): "B로 진행" — 노트는 이때 ApplyDamage 인자 추가는 원치 않는다는 판단도 함께 적는다.

> 사용자 2026-09-23 (반환값 제안): "반환값으로 처리하면 되지 않나요?"

> 사용자 2026-09-23 (추가 효과 Spec 시점): "반응이 끝난 뒤에 만들어야해요."

노트가 요약으로만 남긴 판단: 사망 확인은 Effect 내부로, 적대 판정은 ApplyDamage에 유지(A), 방어 판정 ExecCalc 이관과 추가 효과 Effect 이관(B안 전체) 승인. `Wx.Combat.Damage.Result` 자동화 테스트 삭제도 사용자 지시였다.

## 문제 진단

반응이 결과를 모르는 Hit GE의 `OnGameplayEffectApplied`에 있어서 Damage GE 실행 결과를 Context로 거꾸로 올려야 했다. 이 역방향 통로가 `FWxHitEffectContext`·Result 수치·결과 태그 사본·Reset/Duplicate/NetSerialize 규칙을 낳았다.

## 최종 구조 (노트 마지막 상태 기준, 구현 관찰)

- `ApplyDamage(Causer, Target, DamageTableRow, HitResult)`는 **Damage GE 적용 여부(bool)** 만 반환한다. 판정·수치는 싣지 않는다. `FWxDamageResult`는 삭제.
- 삭제: `UWxEffect_Hit`, `UWxEffectComponent_Hit`, `FWxHitEffectContext`, `Damage.Attack` 태그(항상 붙어 의미 없음), `Event.DodgeSuccess` 발행.
- 적대 판정은 ApplyDamage에 남는다. GE `CanApply`로 옮기면 Immunity 쿼리가 먼저 돌아 아군 공격에도 회피 통지가 나가므로 되돌렸다.
- 사망 확인은 ApplyDamage에서 제거(Death 어빌리티가 `Ability.*`를 취소해 Dodge가 비활성).
- 방어 판정은 ExecCalc가 대상 태그로 내리고 결과 태그(`Damage.Guarded`/`Damage.PerfectGuarded`)를 붙인다. 퍼펙트 가드면 IncomingReflect만 출력한다.
- Damage GE 컴포넌트(생성자 추가 순서 = 실행 순서): `UWxEffectComponent_DamageReaction`(플로터·Hit Cue·가드 취소·피격·가해, 피해량>0 기준) → `_PerfectGuard`(이벤트·반사 GP·패리·Cue·투사체 되돌림) → `_HitStop`(EffectCauser를 무기/투사체로 캐스트해 InstigatorHitStop/VictimHitStop) → `_AdditionalEffects`(퍼펙트 가드가 아니면 적용).
- 추가 효과: 입력 전용 `FWxDamageEffectContext`(서버 로컬, Duplicate 시 비움)가 GE 클래스 목록만 싣고, `_AdditionalEffects`가 **반응 뒤** 실행 시점에 같은 Context로 Spec을 만든다.
- 회피: Dodge 어빌리티가 활성 동안 ASC의 `OnImmunityBlockGameplayEffectDelegate`를 구독하고, 막힌 Spec의 Def가 `UWxEffect_Damage`이면 극한 회피로 전환한다(EndAbility에서 해제). 무적은 항상 `UWxEffect_Invincible`(Immunity 포함)로 부여된다. 투사체는 적용 전에 무적 태그로 통과를 정한다.
- 사망/그로기 발행은 AttributeSet에 유지(치트·AddGP 경로 공유).

## 설계 판단 근거

- 별도 내부 GE로 쪼개지 않은 이유: 새 적용마다 Spec 복사·중첩이 생기고 Damage GE 실행 기록을 읽지 못한다.
- 엔진 `ConditionalGameplayEffects`와 `UAdditionalEffectsGameplayEffectComponent`는 GE 클래스 단위 정적 목록이라 피해 행별 추가 효과·퍼펙트 가드 생략을 대체하지 못한다.
- 엔진 근거(UE 5.8): `ExecuteActiveEffectsFrom`은 모디파이어·AttributeSet 훅 뒤 OnExecuted를 호출한다. Immunity 방송은 `ApplyGameplayEffectSpecToSelf`에서 `CanApply`보다 먼저 돈다.

## 동작 변화

- 추가 효과가 대상 활성 GE 잠금 밖에서 실행된다. 반응이 Damage GE의 OnApplied·적용 델리게이트보다 먼저 실행된다(구독자 없음).
- 히트스톱이 추가 효과보다 먼저, 투사체 되돌림이 퍼펙트 가드 반응 안에서 일어난다. 히트스톱 값은 모두 C++ 기본값(무기 0.1/0.1, 투사체 0/0.1)이었다.

## 미결정·충돌

- 공격별 히트스톱이 필요해지면 피해 행+SetByCaller로 옮기는 안(A)을 검토한다(미결정).
- 적대 판정을 GE로 옮기지 못한 제약은 남아 있다. 노트는 현재 콘텐츠의 모든 호출 경로가 사전 적대 필터를 가져 실위험은 낮다고 본다.

## 검증 범위

- 테스트는 반환값과 자원 변화로 검증하고, 반사량 0 퍼펙트 가드만 이벤트 관찰로 확인한다고 적었으나, 이후 사용자 지시로 `Wx.Combat.Damage.Result` 자동화 테스트 자체가 삭제됐다. 노트에는 빌드·인게임 결과가 없다.

## 관련 주제

- [[피해 파이프라인]]
- [[어빌리티와 GAS]]
- [[그로기·경직·피니시]]

## 핵심 주장

- 2026-09-23 사용자 승인으로 Hit Wrapper GE와 FWxHitEffectContext를 삭제하고 ApplyDamage 판정 → Damage GE → 반응 컴포넌트로 결과가 앞으로만 흐르게 재설계했다. ^c1
- 2026-09-23 사용자 지시로 회피는 Dodge 어빌리티가 ASC의 OnImmunityBlockGameplayEffectDelegate를 구독해 막힌 Damage GE를 감지하는 방식으로 재구현됐다. ^c2
- 2026-09-23 최종 상태에서 ApplyDamage는 FWxDamageResult 대신 Damage GE 적용 여부 bool만 반환한다. ^c3
- 2026-09-23 사용자 지시로 추가 효과 Spec은 피해 반응이 끝난 뒤 _AdditionalEffects 컴포넌트가 실행 시점에 만든다. ^c4
