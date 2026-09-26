---
type: source
title: "결정 노트 - 2026-09-23-zero-damage-hitstop"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "피해"
  - "히트스톱"
  - "GAS"
summary: "히트스톱을 Hit Cue와 같은 조건(피해 0 초과 또는 퍼펙트 가드)으로 맞추고 Hit Cue 예측 발행 등 낡은 주석을 정정한 기록. 빌드 통과, 플레이 미검증."
source_type: decision-note
source_id: src-c50b38fd4c3e9da6a2b4
sha256: c2a0caa9fce4784c6ca575f588760e258d99ff69c8e237776d00c6f2c9ce89e9
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-zero-damage-hitstop.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-zero-damage-hitstop.md"
raw_copy: ".raw/captured/c2a0caa9fce4784c6ca575f588760e258d99ff69c8e237776d00c6f2c9ce89e9.md"
claim_ids:
  - clm-a3d8a22a8a-c1
  - clm-a3d8a22a8a-c2
  - clm-a3d8a22a8a-c3
  - clm-a3d8a22a8a-c4
key_claims:
  - "2026-09-23 이후 UWxEffectComponent_HitStop은 IncomingDamage가 0보다 크거나 Damage.PerfectGuarded 태그가 있을 때만 히트스톱을 건다."
  - "WX의 Hit Cue는 공격자 클라 예측 발행이 아니라 _DamageReaction이 서버에서 빈 예측 키로 발행하므로 공격자 클라도 서버 판정 뒤에 받는다."
  - "UWxEffect_Invincible의 Immunity는 UWxEffect_Damage 클래스만 막으므로 이미 걸린 지속 피해 GE는 무적 중에도 들어간다."
  - "0 피해 히트스톱 변경은 WxEditor 빌드만 통과했고 플레이 검증은 하지 않았다."
---

# 결정 노트 - 2026-09-23-zero-damage-hitstop

- 원본: `.wiki/raw/notes/2026-09-23-zero-damage-hitstop.md`
- 원자료 사본: `.raw/captured/c2a0caa9fce4784c6ca575f588760e258d99ff69c8e237776d00c6f2c9ce89e9.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-zero-damage-hitstop.md` (source: MANUAL, ingested: 2026-09-23).
- 다른 GAS 프로젝트(Lyra·Action RPG·GASDocumentation·Aura·Ninja Combat 등)와 비교한 개선 제안 중 1(낡은 주석 정정)과 2(0 피해 히트스톱)를 적용한 기록이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-23: "1, 2 적용해주세요"

## 구현 관찰: 히트스톱 조건

- `UWxEffectComponent_HitStop`은 Damage GE 실행 기록의 `IncomingDamage`가 0보다 크거나 Spec 동적 태그에 `Damage.PerfectGuarded`가 있을 때만 히트스톱을 건다. `UWxEffectComponent_DamageReaction`의 Hit Cue와 같은 조건이다.
- 이전 동작: ExecCalc가 `FinalDamage <= 0`으로 출력 없이 끝난 타격(반올림 0, 완전 경감 가드)은 플로터·Hit Cue·피격 이벤트·가드 SP 차감이 모두 빠졌는데 히트스톱만 걸렸다.
- `UWxEffectComponent_AdditionalEffects`는 피해 없는 디버프 행을 위해 0 피해에도 적용을 유지한다(퍼펙트 가드만 생략).

## 구현 관찰: 주석 정정

- Hit Cue 발행: `WxCueNotify_Hit`·`WxCueNotify_DamageFloater` 헤더의 "공격자 클라에서 예측 발행" 서술은 사실이 아니었다. Damage GE에는 Cue 정의가 없고 `_DamageReaction`이 서버에서 빈 예측 키로 발행하므로 공격자 클라도 서버 판정 뒤에 받는다.
- 무적 범위: `UWxEffect_Invincible`의 Immunity는 `UWxEffect_Damage` 클래스만 막고, 그 차단 통지를 Dodge가 극한 회피로 받는다. 추가 효과로 이미 걸린 지속 피해 GE나 치트 `UWxEffect_AddIncomingDamage`는 무적 중에도 들어간다.
- 무기 사전 검사: `AWxWeaponBase::ProcessHit`의 적대 사전 검사는 `ApplyDamage`의 적대 판정과 중복이다. 낡은 이유 주석만 삭제하고 검사는 유지했다.

## 검증 범위

- 전체 WxEditor 빌드 성공(경고 0), 로그 `Saved/Logs/BuildDoctor/build_2026-09-23_204153_209_33376.log`.
- 플레이 미검증. 남은 기획 확인 항목은 `.agents/workflow/tasks/damage-pipeline-structure-review.md`에 있다고 적는다.

## 관련 주제

- [[피해 파이프라인]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-23 이후 UWxEffectComponent_HitStop은 IncomingDamage가 0보다 크거나 Damage.PerfectGuarded 태그가 있을 때만 히트스톱을 건다. ^c1
- WX의 Hit Cue는 공격자 클라 예측 발행이 아니라 _DamageReaction이 서버에서 빈 예측 키로 발행하므로 공격자 클라도 서버 판정 뒤에 받는다. ^c2
- UWxEffect_Invincible의 Immunity는 UWxEffect_Damage 클래스만 막으므로 이미 걸린 지속 피해 GE는 무적 중에도 들어간다. ^c3
- 0 피해 히트스톱 변경은 WxEditor 빌드만 통과했고 플레이 검증은 하지 않았다. ^c4
