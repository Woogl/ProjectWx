---
title: "타격별 피해 실행 정의 스냅샷"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage, static-review]
summary: "피해 행을 한 번 해석해 계수·공격 태그·추가 효과를 보존하고 Hit 내부 재조회를 제거했다."
---

# 타격별 피해 실행 정의 스냅샷

2026-09-23 미커밋 작업 트리의 아래 심볼을 조사했다. 실행 검증은 Workflow Task에 별도로 기록한다.

- `ApplyDamageRequest`는 피해 행을 한 번 조회한다. `FWxDamageTableRow::MakeHitSpec`은 외부 Spec 생성 호출 전에 계수·공격 태그·추가 효과 클래스를 `FWxDamageDefinition`에 복사한다.
- Hit Spec의 SetByCaller와 DynamicAssetTags는 이 정의에서 만든다. `UWxEffectComponent_Hit`는 정의의 유효 상태를 검사하고 복사된 추가 효과 목록을 소비한다. 테이블을 다시 조회하지 않는다.
- `FWxHitEffectContext`의 정의는 로컬 실행용이다. Duplicate는 정의를 복사하고 실행 결과를 지운다. NetSerialize 수신은 정의를 지우며 클라이언트에서 재해석하지 않는다. 기존 테이블/행 이름의 전송은 진단 및 형식 호환을 위해 유지한다.
- ATK/DEF 및 방어 상태는 정의에 포함하지 않는다. 추가 효과 Spec의 태그 캡처 시점과 피해·반응 순서는 기존과 같다. 다음 요청은 당시 행을 새로 해석한다.
- 공개 BP 함수와 DataTable 필드 형식은 유지한다. C++에서 직접 Hit Spec을 만들 때도 MakeHitSpec을 통해 정의를 설정해야 한다.

## 코드 근거

- [실행 정의](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageDefinition.h): 전체 필드.
- [해석과 Spec 생성](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp): MakeHitSpec.
- [Context](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp): Duplicate, NetSerialize.
- [Hit 소비](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp): CanGameplayEffectApply, OnGameplayEffectApplied.
- [회귀 검사](../../../Plugins/WxCombat/Source/WxCombat/Private/Tests/WxDamageResultTest.cpp): 행 삭제 후 기존 Spec 적용, 다음 요청의 변경 행 반영.
