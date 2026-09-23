---
title: "소환물 주인 태그를 State.MinionMaster.*에서 Master.*로 변경"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, combat, foundation]
summary: "소환물을 보유한 주인 ASC에 붙는 태그를 State.MinionMaster.Minion·Doppelganger에서 Master.Minion·Master.Doppelganger로 바꿨다. 태그 리다이렉트 없이 소환물 BP와 HGTest 어빌리티 에셋을 다시 저장했다."
---

# 소환물 주인 태그 이름 변경

2026-09-24 커밋 `c4dee8382`. 커밋 메시지에 이유는 적혀 있지 않다. HEAD `d76e48717` 코드로 대조했다.

## 변경

- 네이티브 태그 `State.MinionMaster.Minion`·`State.MinionMaster.Doppelganger`를 `Master.Minion`·`Master.Doppelganger`로 바꿨다(`WxGameplayTags::Master_Minion`·`Master_Doppelganger`).
- 의미는 그대로다. 소환물을 보유한 주인에게 붙는다. 같은 입력을 쓰는 소환·명령 스킬의 발동 조건이며, 부모 `Master`로 물으면 종류를 가리지 않는다.
- 발행: `UWxMinionComponent::MasterStateTag`(C++ 기본값 `Master.Minion`, 도플갱어는 BP에서 `Master.Doppelganger` 지정)가 종류를 정한다. 로스터가 바뀔 때마다 `UWxMinionSubsystem::RefreshMasterStateTag`가 권위 머신에서 주인 ASC에 loose 태그를 `TagOnly` 복제로 세우거나 내린다. 같은 태그 값은 소환 상한을 다투는 단위이기도 하다.
- 다시 저장한 에셋: `BP_Minion`, `BP_Doppelganger`, HGTest 어빌리티 9개(`GA_HGTest_Attack_*`·`Skill_*`·`Ultimate_*`), `GE_HGTest_AddUP`. `Config`에 GameplayTag 리다이렉트는 없다.
- `Content`·`Plugins/*/Content` 에셋에서 `MinionMaster` 문자열은 더 나오지 않는다(2026-09-24 grep).

## 검증 범위

코드 정적 대조와 에셋 문자열 검색만 했다. 소환·명령 스킬의 발동 조건과 소환 상한 동작은 인게임에서 확인하지 않았다.

근거: [태그 선언](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h), [소환물 컴포넌트](../../../Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp), [소환물 서브시스템](../../../Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp).
