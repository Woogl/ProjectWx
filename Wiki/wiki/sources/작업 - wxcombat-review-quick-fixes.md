---
type: source
title: "작업 - wxcombat-review-quick-fixes"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "WxCombat"
  - "코드-리뷰"
  - "GAS"
summary: "WxCombat 모듈 리뷰 지적 중 몽타주 동기 재생 실패의 성공 반환과 권위 없는 머신의 피해 판정 쿼리를 고친 2026-09-30 완료 작업 기록"
source_type: task-record
source_id: src-ec22ecc42c819fc7cc91
sha256: 50bd0f194a597fcd7c5ca14af99665739bcdeb50cd0e8ef5dfa872a9ade7fdee
authority: primary
independence_key: ".agents/workflow/tasks/wxcombat-review-quick-fixes.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/wxcombat-review-quick-fixes.md"
raw_copy: ".raw/captured/50bd0f194a597fcd7c5ca14af99665739bcdeb50cd0e8ef5dfa872a9ade7fdee.md"
claim_ids:
  - clm-50bd0f194a-c1
  - clm-50bd0f194a-c2
  - clm-50bd0f194a-c3
  - clm-50bd0f194a-c4
key_claims:
  - "2026-09-30 수정으로 UWxAbilityBase::PlayMontageInternal은 태스크 활성화 뒤 IsActive()를 반환해, 몽타주 동기 재생 실패 시 호출자가 false를 받아 띄우기·회전·후속 태스크를 건너뛴다."
  - "2026-09-30 수정으로 AWxWeaponBase::BeginAttack과 UWxAnimNotify_AreaDamage::Notify는 무기가 아니라 소유 공격자 캐릭터에 권위가 없으면 피해 판정을 시작하지 않는다."
  - "WxCombat 리뷰 지적 2(겹친 슬로모션이 전역 배율을 먼저 되돌림)는 월드 단위 배율 요청 관리처와 겹침 정책이 필요해 2026-09-30 수정 범위에서 보류됐다."
  - "WxCombat 리뷰 수정은 Editor 빌드와 LV_DevCombat 헤드리스 임시 테스트(수정 전 대조 포함, 비권위는 로컬 역할 전환으로 재현)로 확인했고 woogle이 2026-09-29 코드 리뷰를 통과시켜 체크리스트 4/4로 완료됐다."
---

# 작업 - wxcombat-review-quick-fixes

- 원본: `.agents/workflow/tasks/wxcombat-review-quick-fixes.md`
- 원자료 사본: `.raw/captured/50bd0f194a597fcd7c5ca14af99665739bcdeb50cd0e8ef5dfa872a9ade7fdee.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

`module_review_WxCombat.md`(기준 커밋 `e09ed19d1`, 2026-09-30)의 지적 1(몽타주 동기 재생 실패를 성공으로 반환)과 지적 3(권위 없는 머신의 피해 판정 쿼리)을 고친 작업 기록이다. 이우성이 "손쉽게 해결 가능할까요?"라고 묻고 AI의 1·3번 수정 제안에 "네"로 승인했다. 지적 2(겹친 슬로모션이 전역 배율을 먼저 되돌림)는 월드 단위 배율 요청 관리처와 겹침 정책이 필요해 AI가 보류를 권했고 범위에 넣지 않았다. 상태는 완료(체크리스트 4/4 통과)다.

## 구현 결과

- 지적 1: `UWxAbilityBase::PlayMontageInternal`이 태스크 활성화 뒤 `true` 대신 `IsActive()`를 반환한다. 엔진 `UAbilityTask_PlayMontageAndWait::Activate`는 재생 실패 시 같은 호출 안에서 `OnCancelled`를 방송하고 베이스가 어빌리티를 끝내므로, 호출자가 실패를 `false`로 받아 띄우기·회전·후속 태스크를 건너뛴다.
- 지적 3: `AWxWeaponBase::BeginAttack`과 `UWxAnimNotify_AreaDamage::Notify` 입구에서 공격자 캐릭터에 권위가 없으면 판정을 시작하지 않는다. 무기 자신이 아니라 소유 캐릭터로 판단한다(복제하지 않는 차일드 액터 무기는 각 머신이 로컬로 만들어 클라이언트에서도 자신이 권위다).
- 사이드 이펙트 검토(기록의 분석): 지적 1은 엔진이 재생을 동기로 거부할 때만 반환이 바뀌고, 취소 처리에서 끝내지 않는 `UWxAbility_Death`는 여전히 `true`이며 `UWxAbility_Groggy`는 자체 재정의라 영향이 없다. 지적 3은 판정 결과의 유일한 소비처 `ApplyDamage`가 이미 비권위 출처를 거절하므로 단독·리슨 호스트·데디 서버 동작은 같고 클라이언트의 버려지던 쿼리만 사라진다. 클라이언트에서 범위 피해 타게팅의 `ts.debug` 드로잉은 더는 보이지 않는다.
- 변경 파일: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`.

## 검증 범위

- AI: WxEditor Development 빌드 성공(임시 테스트와 WxGame.Build.cs 임시 의존을 되돌린 뒤).
- AI 헤드리스(`LV_DevCombat -game -nullrhi`, 임시 자동화 테스트 뒤 삭제): HitReact KnockUp이 정상 재생 때 Z 640으로 띄우고, AnimInstance를 비워 동기 실패시키면 어빌리티가 끝나고 띄우지 않았다. 수정 전 대조는 끝난 어빌리티가 띄워 실패로 잡혔다.
- AI 헤드리스: 적의 로컬 역할을 SimulatedProxy로 바꾸면 무기 틱·형상 충돌·타게팅 요청이 0이고, Authority에서는 지금처럼 켜졌다 구간 끝에 꺼지고 요청 1회였다. 실제 리슨 서버·클라이언트 PIE가 아니라 로컬 역할 전환으로 비권위를 만들었다.
- 사람: woogle 2026-09-29 코드 리뷰 통과.

## 관련 주제

- [[어빌리티와 GAS]]
- [[피해 파이프라인]]
- [[모듈 구조와 코드 정리]]

## 핵심 주장

- 2026-09-30 수정으로 UWxAbilityBase::PlayMontageInternal은 태스크 활성화 뒤 IsActive()를 반환해, 몽타주 동기 재생 실패 시 호출자가 false를 받아 띄우기·회전·후속 태스크를 건너뛴다. ^c1
- 2026-09-30 수정으로 AWxWeaponBase::BeginAttack과 UWxAnimNotify_AreaDamage::Notify는 무기가 아니라 소유 공격자 캐릭터에 권위가 없으면 피해 판정을 시작하지 않는다. ^c2
- WxCombat 리뷰 지적 2(겹친 슬로모션이 전역 배율을 먼저 되돌림)는 월드 단위 배율 요청 관리처와 겹침 정책이 필요해 2026-09-30 수정 범위에서 보류됐다. ^c3
- WxCombat 리뷰 수정은 Editor 빌드와 LV_DevCombat 헤드리스 임시 테스트(수정 전 대조 포함, 비권위는 로컬 역할 전환으로 재현)로 확인했고 woogle이 2026-09-29 코드 리뷰를 통과시켜 체크리스트 4/4로 완료됐다. ^c4
