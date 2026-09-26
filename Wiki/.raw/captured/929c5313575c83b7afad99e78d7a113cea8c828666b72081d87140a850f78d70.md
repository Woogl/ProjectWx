---
title: "ApplyDamage 반환 API 통합"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "bool ApplyDamage와 ApplyDamageWithResult를 FWxDamageResult 반환 ApplyDamage 하나로 합쳤다."
---

# ApplyDamage 반환 API 통합

사용자가 두 함수의 통합을 요청했다. `UWxCombatLibrary::ApplyDamage`가 `FWxDamageResult`를 반환하고 `ApplyDamageWithResult` 및 bool 래퍼는 제거했다. 성공 여부만 필요한 호출은 `.bApplied`를 읽는다.

출처를 직접 지정하는 C++ `ApplyDamageRequest`는 유지한다. ApplyDamage의 출처 추론/피해 처리 정책은 그대로이며 반환 API만 통합했다. 기존 GAS 테스트를 통합 함수로 전환했다.

Source/Plugins C++ 검색에서는 두 기존 함수의 호출자가 자동화 테스트뿐이었다. Content uasset 문자열 검색에서 두 함수 이름 참조는 없었다. 외부 Blueprint 사용처가 있다면 기존 bool 반환 핀은 새 결과의 bApplied로 연결하고 ApplyDamageWithResult 노드는 ApplyDamage로 바꿔야 한다. 검색을 전체 Blueprint 실행 검증으로 확대하지 않는다.

근거: [공개 API](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h), [구현](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageCompatibility.cpp), [회귀](../../../Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp). 검증 결과는 Workflow Task에 기록한다.
