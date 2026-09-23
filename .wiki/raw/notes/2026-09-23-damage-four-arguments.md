---
title: "ApplyDamage 네 인자 인터페이스 복원"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "사용자 승인으로 요청 구조체를 제거하고 Causer/Target/피해 행/HitResult 인자를 복원했다."
---

# ApplyDamage 네 인자 인터페이스 복원

사용자가 이전 인자의 편의성을 선호했고 출처·Ability·레벨 추론을 복원하는 변경을 승인했다.

- 유일한 진입점은 `FWxDamageResult ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult)`다. 반환값은 평탄화한 결과를 유지한다.
- FWxDamageRequest를 제거하고 네이티브 네 호출부의 요청 조립을 없앴다. 별도 래퍼나 오버로드는 추가하지 않는다.
- Causer의 ASC를 먼저 찾고 없으면 직접 Owner의 ASC를 사용한다. Source와 Target의 ASC 및 출처 권위 검사는 유지한다.
- 투사체는 저장된 ProjectileLevel과 Ability=nullptr을 사용한다. 반사로 Owner가 바뀌면 현재 Owner의 ASC를 쓰고 발사 레벨은 유지한다. 일반 공격은 적용 직전 AnimatingAbility와 그 레벨, 없으면 1을 사용한다.
- 테스트의 명시 요청 시나리오를 실제 템플릿 투사체의 Owner/저장 레벨/Owner 교체 검사로 바꿨다. 피니셔·무기·범위 공격의 계산과 이벤트 순서는 유지한다.
- 중간 요청 구조체 API를 사용하는 외부 Blueprint가 있다면 네 인자 노드로 전환해야 한다. 최초 Content 검색에서는 ApplyDamage 참조가 없었다.

근거: [API](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h), [실행](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp), [회귀](../../../Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp). 실행 검증은 Workflow Task에서 관리한다.
