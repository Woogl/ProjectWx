---
title: "피해 입력 보관 구조 단순화"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "사용자 검토에 따라 DamageDefinition과 유효 플래그를 제거하고 기존 Spec과 Context에 필요한 값만 둔다."
---

# 피해 입력 보관 구조 단순화

사용자가 별도 DamageDefinition의 복잡성을 지적했고, 계수·태그는 기존 Spec에 두고 추가 효과만 Context에 복사하는 변경을 승인했다. 앞선 실행 정의 원자료의 별도 타입과 유효 상태 설명을 대체한다.

- FWxDamageDefinition과 bHasDefinition을 제거했다. MakeHitSpec은 기존 SetByCaller와 DynamicAssetTags에 계수·태그를 설정한다.
- FWxHitEffectContext에는 AdditionalEffects 배열만 복사한다. Hit 컴포넌트는 이 목록을 소비하고 테이블을 다시 읽지 않는다. 유효한 피해 행의 확인은 ApplyDamageRequest 진입점에서 한 번 수행한다.
- Context Duplicate는 목록을 복사하며 NetSerialize 수신에서는 목록을 비운다. 목록은 서버 로컬 실행용이다.
- 피해 계산·방어 판정·이벤트 순서 및 기존 테이블 식별자의 전송 형식은 유지한다.

근거: [MakeHitSpec](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp), [Context](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxHitEffectContext.h), [Hit](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp). 검증 결과는 Workflow Task에 기록한다.
