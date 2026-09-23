---
title: "WxAI 리뷰 후속: 미니언 노드 삭제·도플갱어 이동속도 SPD 소유·Mirror 노드 정리"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ai, architecture]
summary: "WxAI 모듈 리뷰 후속으로 쓰이지 않는 미니언 반응 노드를 지우고, 도플갱어 이동속도를 SPD Override GE로 Master를 따르게 바꾸고, Mirror 노드·타겟 서비스 헬퍼를 정리했다. 이동속도는 SPD가 소유하고 BT 노드는 GE로만 바꾼다. 사용자가 인게임 동작을 확인했다."
---

# WxAI 리뷰 후속 수정

사용자 지시(2026-09-23): 리뷰 1번 "삭제 진행하세요", 2번 "이동속도를 Master 추종하게 하세요", 4·5번 "고쳐주세요". 3번(ObserveAbility의 InstancedPerExecution 발동 누락)은 "InstancedPerExecution인 어빌리티를 안 쓰기 때문"이라며 수용했다. 사용자가 테스트 결과 이상 없음을 확인했다.

## 삭제한 미니언 반응 노드

- `UWxBTService_ObserveMasterAbility`·`UWxBTDecorator_MasterAbility`·`UWxBTTask_FollowMasterAbility`와 `WxBlackboardKeys::GetMaster`를 지웠다. 2026-09-19 이관(커밋 `e8b724d72`)이 `UWxBTDecorator_ObserveAbility`로 대체한 뒤 남긴 잔재였다. `Content/` 전체 uasset·umap 문자열 검색에서 참조가 없었다.
- `Master` 키와 `SetMaster`는 `AWxAIController`가 쓰므로 유지한다.

## 이동속도 소유

- `AWxCharacterBase`가 `MaxWalkSpeed`를 클래스 기본 CMC `MaxWalkSpeed` × SPD로 쓴다(SPD 변경 콜백). BT 노드는 `MaxWalkSpeed`를 직접 쓰지 않고 SPD를 바꾸는 GE로만 속도를 바꾼다.
- Patrol·Wander는 `UWxEffect_MoveSpeedScale`(MultiplyCompound, `SetByCaller.MoveSpeedScale`)를 BT에서 지정받아 곱한다.
- `UWxBTService_MirrorMovement`는 `MoveSpeedEffect`로 지정받은 `UWxEffect_MoveSpeedOverride`(Override, 같은 SetByCaller 태그)를 도플갱어에 건다. 값은 Master의 현재 `MaxWalkSpeed` × 1.25 ÷ 도플갱어 클래스 기본 `MaxWalkSpeed`이며, 바뀌면 `UpdateActiveGameplayEffectSetByCallerMagnitude`로 갱신한다. 엔진 aggregator 갱신은 배치 범위 밖에서 동기라 같은 프레임에 도플갱어 속도가 바뀐다. 서비스가 끝나면(`Release`) GE를 제거해 SPD 콜백이 원래 속도를 되돌린다.
- 앉은 속도는 SPD가 다루지 않는다. Mirror는 Master 앉은 속도 × 1.25를 직접 쓰고, 종료 시 클래스 기본값으로 되돌리며 `UnCrouch`한다.
- Override를 쓴 이유: 도플갱어가 따라 쓴 Sprint는 끝나지 않는다. `UWxAbility_Sprint`는 `InputReleased`에서만 종료하고 `UWxBTTask_MirrorAbility`는 커밋만 따라 하므로, AI에서는 SP가 바닥날 때까지(SP가 없으면 영구) ×1.5가 남는다. 곱연산은 이 자기 SPD 효과를 살려 속도가 어긋난다. Override는 같은 채널의 다른 모든 SPD 효과를 무시한다(`FAggregatorModChannel::EvaluateWithBase`).
- 기각한 대안: `MaxWalkSpeed` 직접 쓰기 + 복원은 WxAI가 WxCombat을 참조하지 않아 SPD를 읽지 못해 복원값을 계산할 수 없다. BP 기본 속도 ×1.25 고정은 Master의 비어빌리티 SPD 변화를 따르지 못한다.
- 남은 제약: 도플갱어에 남는 Sprint의 `Ability.Sprint` 태그와 SP 소모는 그대로다. 속도에는 영향이 없고 다른 영향은 확인하지 않았다.

## Mirror 노드·타겟 서비스 정리

- 두 Mirror 노드의 기본 키를 `WxBlackboardKeys::Master`로 바꿨다. AI 컨트롤러는 클라이언트에 스폰·복제되지 않아 BT가 서버에만 있으므로 항상 참인 `HasAuthority()` 검사 3곳을 지웠다.
- `UWxBTTask_MirrorAbility::ExcludedAbilities`를 `FGameplayTagContainer` 하나로 바꿨다. `BT_Doppelganger`의 기존 값 `[{Ability.Finisher}, {Ability.Ultimate}]`를 `{Ability.Finisher, Ability.Ultimate}`로 옮기고, Mirror Movement에 `MoveSpeedEffect = WxEffect_MoveSpeedOverride`를 지정했다. `FaceMasterAbilityTags`는 `{Ability.Skill.3}`이다.
- `WxBlackboardKeys::VerifyBlackboardKey`를 익명 namespace에서 헤더 선언(비export)으로 옮겼다. `UWxBTService_UpdateTargetActor`의 `IsActorDead`·`CanBeAggroTarget`을 지우고 두 호출부에서 ASC를 한 번 조회해 `Ability.Death`·`Effect.IgnoreAggro`를 본다. 판정은 같다.

근거: [MirrorMovement](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp), [MirrorAbility](../../../Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp), [Override GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_MoveSpeedOverride.cpp), [SPD 콜백](../../../Source/WxGame/Character/WxCharacterBase.cpp), [Sprint](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp), [타겟 서비스](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp), [Blackboard 키](../../../Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h).

검증: 커밋 `411e74d7f`·`7c4420fbb`·`fab9b004c`·`0aaaa917d`. WxEditor 빌드 성공, 자동화 테스트 `Wx.AI`(2건) 통과, 헤드리스 에디터로 `BT_Doppelganger` 저장값을 다시 읽어 확인했다. 사용자가 인게임 테스트 이상 없음을 확인했다(2026-09-23). 리뷰 기록은 [작업 자료](../../../.agents/workflow/tasks/module_review_WxAI.md)에 있다.
