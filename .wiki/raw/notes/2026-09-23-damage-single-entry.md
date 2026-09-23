---
title: "Damage의 단일 요청 진입점"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "ApplyDamage(Request) 하나로 C++과 Blueprint 피해 호출을 통합하고 출처 추론 어댑터를 제거했다."
---

# Damage 단일 진입점

사용자가 ApplyDamageRequest도 제거해 진입점을 하나로 요청했다. 직전 두 반환 API 통합을 이어서, `ApplyDamage(const FWxDamageRequest&) -> FWxDamageResult`를 유일한 공개 피해 호출로 사용한다.

- ApplyDamageRequest의 실행 본문을 ApplyDamage로 통합했다. 네 가지 네이티브 호출자와 GAS 테스트가 같은 함수를 호출한다.
- FWxDamageRequest를 BlueprintType USTRUCT와 편집 가능한 UPROPERTY 필드로 노출했다. UObject 필드는 TObjectPtr이며 Target/SourceAbility는 const를 유지한다. 동기 호출이며 지연 요청용 능력치 스냅샷은 아니다.
- 기존 다중 인자 ApplyDamage와 출처 추론 어댑터 WxDamageCompatibility.cpp를 제거했다. 요청에 SourceASC/Instigator/Causer/Target/Ability/레벨/행/HitResult를 명시한다. 투사체 레벨과 현재 Ability 선택 정책은 기존 호출자에 남는다.
- Blueprint도 Make WxDamageRequest 또는 구조체 핀 분할로 같은 입력을 전달한다. 기존 다중 인자 노드는 요청 핀으로 전환해야 한다. 기존 Content 문자열 검색에서는 해당 노드 참조가 발견되지 않았다.
- 별도 Blueprint 래퍼나 오버로드는 추가하지 않았다.

근거: [공개 API](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h), [요청](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageRequest.h), [실행](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp). 빌드와 회귀 결과는 Workflow Task에 기록한다.
