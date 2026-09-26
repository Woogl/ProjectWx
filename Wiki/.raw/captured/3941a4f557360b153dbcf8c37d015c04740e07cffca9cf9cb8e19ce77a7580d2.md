---
title: "Hit 처리의 함수별 책임"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "새 타입 없이 Hit 클래스 안에서 방어 판정, Spec 준비, 결과 기반 반응을 분리했다."
---

# Hit 처리의 함수별 책임

2026-09-23 작업 트리의 UWxEffectComponent_Hit를 기준으로 한다. 사용자 요청은 다음 구조 개선의 자동 진행이며, 직전 검토의 불필요한 타입 추가를 줄이라는 방향을 유지한다.

- EvaluateDefense는 현재 무적/가드 가능/퍼펙트 가드/일반 가드 태그를 읽어 기존 EWxDamageDefense를 반환한다. 이벤트나 자원 변경은 하지 않는다.
- PrepareDamageSpecs는 Linked Damage Spec과 추가 효과 Spec을 준비한다. 추가 효과의 출처 태그는 기존처럼 피해 적용과 반응 이벤트 전에 캡처한다.
- ProcessHitReactions는 확정된 FWxDamageResult를 읽어 Hit Cue, 일반 피격/가해 이벤트, 퍼펙트 가드 반응을 처리한다.
- OnGameplayEffectApplied는 Context 검사, 회피 조기 종료, 피해 적용/실패 종료, 결과 보존, 반응 후 추가 효과 적용 순서를 유지한다.
- 새로운 UObject·데이터 타입·파일 계층은 추가하지 않았다. DamageResponse의 결과 수집과 플로터, 자원 반영 중 사망/그로기 전이 시점은 변경하지 않는다.

근거: [Hit 구현](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp), [선언](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffectComponent_Hit.h), [GAS 회귀](../../../Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp). 실행 검증과 제한은 Workflow Task에 기록한다.
