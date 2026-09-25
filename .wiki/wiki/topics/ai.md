---
title: "WxAI — AI 인지와 행동"
category: topic
sources:
  - "raw/notes/2026-09-22-current-ai.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-wxai-review-followups.md"
  - "raw/notes/2026-09-24-ai-brain-control-single-owner.md"
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, ai]
aliases: ["WxAI"]
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "WxAI는 인지 결과를 Blackboard로 전달하고 Behavior Tree 노드로 이동·전투 행동을 구성한다."
---

# WxAI — AI 인지와 행동

WxAI는 인지 결과를 Blackboard로 전달하고 Behavior Tree 노드로 이동·전투 행동을 구성한다.

## 책임과 조립 경계

WxAI는 AIBehavior 설정·Blackboard 키·BT 서비스/태스크/데코레이터를 제공한다. 실제 AIController는 WxGame에 있다. WxAI의 Build.cs는 GAS와 WxCore에 의존하며 WxCombat을 직접 참조하지 않는다. 전투 속성이나 이동 효과를 요구하는 BT 노드에는 에셋에서 적절한 데이터가 지정되어야 한다.

## 인지에서 행동까지

`UWxAIBehaviorComponent::InitializeComponent`는 빙의가 Pawn Owner를 컨트롤러로 바꾸기 전에 스폰 주체에서 순찰 컴포넌트를 확보한다. BeginPlay에서는 현재 컨트롤러와 이후 교체에 감각 설정을 적용한다. 피해 이벤트는 양수 피해와 적대 관계를 확인해 Damage 센스에 보고한다.

`UpdateTargetActor`는 현재 타겟이 유효하고 사망·IgnoreAggro 상태가 아니면 유지한다. 교체가 필요하면 이전 타겟의 인지 기록을 지운 뒤 감지 목록에서 유효 후보를 찾는다. 이 코드는 가장 가까운 적을 점수화하는 선정기가 아니며, 반환된 감지 목록의 첫 허용 후보를 사용한다.

Blackboard의 SelfActor·HomeLocation·Master와 TargetActor·TargetDistance·PatrolTargetLocation은 공용 키 계약이다. 타겟 부재의 거리를 0으로 초기화하면 근접 조건을 통과할 수 있어 `NoTargetDistance`를 사용한다. 키 이름·타입은 실제 Blackboard 에셋과 맞아야 한다.

## 트리 정지와 잠금

트리를 멈추거나 잠그는 곳은 WxGame의 `AWxAIController` 하나다. 엔진의 일시정지는 단일 플래그라 여러 곳이 걸면 서로의 정지를 풀어 버리기 때문이다. 사망은 캐릭터의 `OnDeath`를 받아 `StopLogic`으로 트리를 끝낸다. 그로기는 `Ability.Groggy` 태그 이벤트를 받아 `LockResource(EAIRequestPriority::Reaction)`로 잠그고 태그가 빠지면 푼다. 엔진 AI 태스크가 쓰는 Logic 잠금과 우선순위가 달라, 그쪽이 풀려도 그로기 중에는 재개되지 않는다. 전투 어빌리티와 돌진 modifier는 트리를 건드리지 않는다. 잠금은 이미 진행 중인 MoveTo 경로 추종과 포커스 회전을 멈추지 않는다.

## 어빌리티 발동

`UWxBTTask_ActivateAbility`는 `AbilityTag`를 에셋 태그로 가진 스펙을 차례로 발동해 처음 성공한 것을 쓴다. 어빌리티가 정상 종료되면 Succeeded, 발동 실패나 취소면 Failed다. BT가 부르는 번호 태그 `Ability.Skill.N`·`Ability.Pattern.N`은 C++ 타입이 아니라 GA_ 에셋이 에셋 태그와 `ActivationOwnedTags`에 더한다. C++ 타입 `UWxAbility_Skill`·`UWxAbility_Pattern`이 기본으로 두는 에셋 태그는 `Ability.Skill`·`Ability.Pattern`뿐이다. `UWxBTDecorator_ObserveAbility`와 `UWxBTTask_MirrorAbility`의 `ExcludedAbilities`도 어빌리티의 에셋 태그를 본다. 원자료에 따르면 PIE에서 솔저 BT가 에셋 태그로 패턴 1·2를 발동했고 소유 태그에 `Ability.Pattern.N`이 실렸다.

## 락온 수명

`UWxBTService_LockOn`은 활성 진입과 틱에 Gameplay 우선순위 포커스를 맞추고 폰의 회전 모드를 설정한다. 분기 이탈 시 포커스를 해제하고 폰의 Movement 아키타입 기본값으로 복구한다. 빙의 해제 뒤에도 이전 폰을 정리할 수 있도록 노드 메모리에 폰을 보관한다. 검색 중 임시 활성화가 아니라 `OnBecomeRelevant`에서 적용하는 이유는 정리 콜백과 수명을 맞추기 위해서다.

## 이동 속도와 도플갱어 미러링

이동 속도의 주인은 SPD다. `AWxCharacterBase`가 클래스 기본 `MaxWalkSpeed`에 SPD를 곱해 쓰므로, BT 노드는 `MaxWalkSpeed`를 직접 쓰지 않고 BT에서 지정받은 GE로 SPD를 바꾼다. WxAI는 WxCombat을 참조하지 않으므로 GE 클래스는 에셋에서 지정한다. Patrol·Wander는 `WxEffect_MoveSpeedScale`로 곱하고 태스크가 끝나면 제거한다.

도플갱어는 `UWxBTTask_MirrorAbility`로 Master가 커밋한 어빌리티를 따라 쓰고, `UWxBTService_MirrorMovement`로 Master 옆을 따라간다. `BT_Doppelganger`는 Finisher·Ultimate를 따라 쓰지 않고, Skill.3 발동 때 Master 앞으로 순간이동해 마주본다. 이동 속도는 `WxEffect_MoveSpeedOverride`로 SPD를 덮어써 Master의 현재 `MaxWalkSpeed` × 1.25에 맞춘다. Master 속도가 바뀌면 SetByCaller 값을 갱신하고, 서비스가 끝나면 GE를 제거해 원래 속도로 돌아간다. 따라서 Master의 질주·SPD 버프·디버프도 같은 프레임에 따라간다.

곱연산이 아니라 덮어쓰기인 이유는 따라 쓴 Sprint가 AI에서 입력 해제를 받지 못해 끝나지 않기 때문이다. 자기 SPD 효과를 살리면 ×1.5가 남는다. 앉은 속도는 SPD가 다루지 않아 Mirror가 직접 쓰고 종료 시 클래스 기본값으로 되돌린다. 도플갱어에 남는 Sprint의 `Ability.Sprint` 태그와 SP 소모가 속도 외에 미치는 영향은 확인하지 않았다.

## 확장과 확인 범위

[Blackboard 계약](../../../Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h), [타겟 서비스](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp), [AIController](../../../Source/WxGame/Controller/WxAIController.cpp) 순서로 추적한다. 도플갱어 미러링은 [이동 서비스](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp)와 [어빌리티 태스크](../../../Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp)에서 시작한다. 순찰·배회·어빌리티 발동 노드도 제공하지만 개별 BT 조합은 이번 조사로 보증하지 않는다. 도플갱어의 속도 추종은 2026-09-23 사용자가 인게임에서 확인했다. 번호 태그를 GA_ 에셋으로 옮긴 뒤(2026-09-25)의 도플갱어 `Skill.3` 반응과 HGTest·분신 BT는 확인하지 않았다.

## 관련 문서

- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))
- [[combat-groggy|그로기]] ([그로기](../concepts/combat-groggy.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-ai.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [근거 3](../../raw/notes/2026-09-23-wxai-review-followups.md)
- [AI 트리 정지·잠금 컨트롤러 단독화](../../raw/notes/2026-09-24-ai-brain-control-single-owner.md) — 사망 정지·그로기 잠금 주체
- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — BT 번호 태그를 GA_ 에셋 태그·소유 태그로, 솔저 BT 패턴 발동 PIE 확인

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

2026-09-23 WxAI 리뷰 후속 수정(커밋 `411e74d7f`~`0aaaa917d`)을 반영했다. 이 범위는 빌드·자동화 테스트·`BT_Doppelganger` 저장값 확인과 사용자 인게임 확인을 거쳤다.

2026-09-24: 트리 정지와 잠금 절을 작업 트리의 `AWxAIController` 코드와 대조해 추가했다. 빌드 통과, 사용자 인게임 확인.

2026-09-25: 어빌리티 발동 절(`UWxBTTask_ActivateAbility`·`UWxBTDecorator_ObserveAbility`·`UWxBTTask_MirrorAbility`의 에셋 태그 조회와 `UWxAbility_Skill`·`UWxAbility_Pattern` 기본 태그)을 HEAD `d63ce0630` 코드와 대조해 추가했고, GA_ 에셋의 번호 태그와 솔저 BT의 PIE 발동은 원자료 기록에만 기대며 직접 검증하지 않았다.

그 밖의 빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
