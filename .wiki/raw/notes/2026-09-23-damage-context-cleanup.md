---
title: "Damage Context의 미사용 데이터와 중복 수치 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "소비자가 없는 테이블 참조의 저장·복제를 제거하고 실행 수치를 Result로 통합했다."
---

# Damage Context의 미사용 데이터와 중복 수치 제거

사용자의 불필요한 구조 제거 요청에 따라 2026-09-23 작업 트리에서 사용처를 확인했다.

- Source/Plugins의 소스 검색상 DamageTable/DamageRowName은 Context 생성과 NetSerialize 외에 읽는 곳이 없었다. 피해 행 조회는 이미 ApplyDamageRequest에서 끝나므로 두 필드와 생성자 행 인자, 테이블 객체 매핑 및 행 이름 직렬화를 제거했다.
- DamageMagnitude/ReflectMagnitude/bHasReflect는 Context와 FWxDamageResult에 중복됐다. DamageResponse는 Result에 수치를 기록하고 Hit는 후속 이벤트 전에 이를 지역 결과로 보존한다. 별도 중간 수치와 초기화 코드를 제거했다.
- 방어 플래그는 실행 계산과 네트워크용이며, 결과 초기화와 수명이 달라 유지한다. 추가 효과 목록, 결과 태그·대상 태그는 실제 소비가 있으므로 유지한다.
- 네트워크 형식은 기본 GameplayEffectContext와 방어 2비트만 남는다. 이전 형식과 비트 호환이 아니므로 같은 빌드의 서버·클라이언트를 사용한다.
- 기존 GAS 회귀에 기본 Context 원점/방어 비트 왕복과 수신 시 로컬 수치/추가 효과 초기화 검사를 추가했다. 실제 네트워크 세션 검증과는 구분한다.

근거: [Context](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp), [결과 수집](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp), [Hit](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp), [테스트](../../../Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp). 빌드·실행 결과는 Workflow Task에 기록한다.
