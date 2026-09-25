---
title: "어빌리티 규칙 변경과 몽타주 섹션 모델(테이블 구동 전환 1-1·1-2단계)"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, combat, animation, inventory, editor]
summary: "콤보 창이 닫히면 첫 단부터, 늦은 노티파이는 몽타주 인스턴스 ID로 거름, 착지 섹션은 재생 중인 모든 몽타주에서 찾음, 그로기 강등은 대미지 반응 컴포넌트, 시체 수명은 캐릭터, 컷신 중 입력형 어빌리티 차단, 소비 아이템은 인벤토리가 고름. 콤보·패턴·회피·가드 반응·피격 반응은 한 몽타주의 섹션으로 고르고, 처형은 한 벌로 합쳐 짝 몽타주·피해 행을 노티파이가 담으며, 궁극기 컷신은 몽타주의 표식 노티파이에서 읽는다."
---

# 어빌리티 규칙 변경과 몽타주 섹션 모델

2026-09-24 커밋 `b4510a45c`·`1f0e0d80f`·`15bcc1682`·`3bc3df8f7`·`f6bb6e5b3`·`1000ea32b`·`ef966ffae`(1-1단계)와 `a8af39cd7`·`a12ca75a0`·`bde81b65b`·`ed40b5663`·`30f873038`·`64483ab39`·`b652ee043`(1-2단계). 작업 기록은 [어빌리티 테이블 구동 전환](../../../.agents/workflow/tasks/ability-table-driven.md)의 "1-1단계"·"1-2단계" 절이다. 2026-09-25 Wiki refresh에서 반영 누락으로 찾았고, 기사 반영 전 HEAD `d63ce0630` 코드와 대조했다. 이후 GA_ 에셋 복귀([어빌리티·GE 데이터를 에셋 한 곳으로](2026-09-25-ability-data-on-ga.md))에서 몽타주 프로퍼티 이름이 `AbilityMontage` 하나로 바뀌었다. Development 빌드 통과, 사용자 플레이 확인("잘 작동되네요" 2026-09-24, 평타 버그 수정 뒤 "이제 해결되었네요").

## 1-1단계: 코드 규칙 변경

- 콤보 재시작(`b4510a45c`): `UWxAbilityBase::CloseComboWindow`가 가상 훅 `OnComboWindowClosed`를 부르고, `UWxAbility_Attack`·`UWxAbility_Skill`이 단계 인덱스를 되돌린다. 창이 닫힌 뒤 누르면 후딜 중이어도 1단부터다(사용자 확정). 이전에는 재발동이 단계를 유지해 후딜 중 입력이 다음 단으로 이어졌다.
- 늦은 노티파이 거르기(`b4510a45c`): 콤보 창·후딜 노티파이가 몽타주 인스턴스 ID를 넘기고, 어빌리티는 지금 재생 중인 인스턴스가 아니면 무시한다(`UWxAbilityBase::IsPlayingMontageInstance`). 끊긴 앞 단 몽타주도 블렌드아웃 동안 노티파이를 보내기 때문이다. 노티파이 두 개는 브랜칭 포인트 경로(`BranchingPointNotify*`)도 오버라이드한다. 엔진 기본 구현이 빈 이벤트 참조를 넘겨 인스턴스를 잃는다.
- 착지(`1f0e0d80f`): `UWxCharacterMovementComponent::JumpToLandingSection`은 가장 최근 몽타주만 보지 않고 재생 중인 몽타주 인스턴스 전부에서 `Grounded` 섹션을 찾는다. 일반 피격이 넉업 위에 겹친 채 착지하면 점프를 놓쳐 `Loop`에 갇히기 때문이다.
- 그로기 강등(`15bcc1682`): 대상이 그로기면 넉 계열 반응 태그를 `HitReact.Normal`로 낮추는 판단을 `UWxEffectComponent_DamageReaction`이 한다. `UWxAbility_HitReact`의 강등 코드는 지웠다.
- 시체 수명(`3bc3df8f7`): `AWxCharacterBase::CorpseLifeSpan`을 두고 `HandleDeath`에서 수명을 건다. `UWxAbility_Death::PendingDestroyTime`은 지웠다. `BP_Minion`만 0.1초다.
- 컷신 중 입력 차단(`f6bb6e5b3`): 새 GE `UWxEffect_SkillCutscene`과 태그 `Effect.SkillCutscene`. `UWxSkillCutsceneComponent`가 세션 시작 때 모든 플레이어 ASC에 이 GE를 걸고 종료 때 스택 하나씩 뺀다. 입력형 어빌리티(Attack·Skill·Ultimate·Dodge·Guard·UseItem·Sprint·LockOn·Interact)를 막고 피격·사망 같은 반응 어빌리티는 막지 않는다. 이동·시점 입력은 시퀀스 재생 설정이 이미 막는다.
- 소비 아이템 선택(`1000ea32b`): `UWxInventoryComponent`의 `CanUseItemByDef`/`UseItemByDef`를 `CanUseConsumable`/`UseConsumable`(요청은 `RequestUseConsumable`)로 바꿨다. 어빌리티의 `ConsumableDef`는 지웠다. 선택 규칙은 Usable 조각이 있고, Charges 조각이 있으면 남은 횟수가 있는 첫 인스턴스다. 재생 전 보유 확인과 꿀꺽 시점 소비 모두 인벤토리에 묻는다.
- 에셋(`ef966ffae` 등): 분신 공격 3개(`GA_Minion_Attack_Heavy`·`Skill_1`·`Skill_2`)의 발동 그룹을 Override에서 Exclusive로 바꿨다. HGTest 스킬 3개의 재발동 금지를 없앴다.

## 1-2단계: 몽타주 섹션 모델

- 몽타주 도구(`a8af39cd7`): `WxAnimMontageToolset`에 `AppendMontage`(원본의 세그먼트·섹션·노티파이를 대상 끝에 이어 붙임), `RenameSection`(섹션 이름과 그 이름을 가리키는 링크를 함께 바꿈), `SnapNotifyEndsToSections`를 더하고 `DescribeMontage`가 노티파이 오브젝트 경로를 돌려준다.
- 콤보·패턴(`a12ca75a0`): 몽타주 하나에 단계 섹션 `1`, `2`, …를 두고 섹션 사이 링크는 끊는다. 단계 수·단계 섹션은 `UWxAbilityBase::GetComboStageCount`·`GetComboStageSection`이 한 곳에서 정한다. 번호 섹션이 없는 몽타주는 처음부터 한 단계다. 단계 전환은 같은 몽타주를 다음 섹션부터 새로 재생해 교차 블렌드를 유지하고, 재생 중인 인스턴스 안에서 `JumpToSection`으로 넘기지 않는다. 콤보는 재발동으로, 패턴은 블렌드아웃 때 다음 섹션을 재생한다. `UWxAbilityBase::PlayMontage`는 없는 시작 섹션이면 경고하고 실패한다(엔진은 무시하고 처음부터 재생한다).
- 회피(`bde81b65b`): 한 몽타주에 8방향 섹션, `Backstep`, 극한 회피 `Success<방향>` 8개를 둔다. 이동 입력이 없으면 `Backstep`, 없으면 `Back`, 구성되지 않은 방향은 `Forward`로 간다. 백스텝·극한 회피용 별도 몽타주 프로퍼티를 지웠다.
- 가드 반응(`ed40b5663`): 몽타주 4개를 `AM_Shared_GuardReact` 하나(섹션 `GuardHit`, `GuardKnockback`, `GuardBreak`, `PerfectGuard`)로 합쳤다. 넉 계열이면 `GuardKnockback`, 아니면 `GuardHit`이다. 넉 계열 몽타주가 없을 때 가드 피격으로 가던 폴백은 없앴다.
- 피격 반응(`30f873038`): 반응 태그의 끝 이름(`Normal`, `KnockBack`, `KnockDown`, `KnockUp`)과 패리(`Event.Hit.Parry` → `Parry`)가 섹션 이름이다. 자기 몽타주에 그 섹션이 있는 어빌리티만 반응한다(`ShouldAbilityRespondToEvent`). 어빌리티는 셋이다.
  - 넉 계열: `AM_Shared_HitReact_Knock`(`KnockBack`, `KnockDown`, `Parry`)
  - 넉업: `KnockUp`(옛 `Default`) → `Loop`, 착지 `Grounded`
  - 일반 피격: 가산 슬롯 `AdditiveHitReact`, `Normal`. 슬롯 그룹이 달라 넉 계열과 합칠 수 없다(엔진은 한 몽타주의 슬롯이 모두 같은 그룹일 때만 재생한다).
  - 수용한 동작 변화: 일반 피격이 넉 계열을 끊지 않고 위에 겹친다. Normal 폴백이 사라졌다.
- 처형(`64483ab39`): 앞잡·뒤잡을 한 벌로 합치고 변형 구조체와 `bBackstab`을 지웠다. 짝 몽타주는 `UWxAnimNotify_FinisherVictim`이, 피해 행은 `UWxAnimNotify_FinisherDamage`가 담는다. 두 노티파이 모두 이벤트에 자기 자신을 싣고(새 태그 `Event.PlayFinisherVictimMontage`), 서버의 처형 어빌리티가 받아 처리한다. `AM_Shared_Finisher` 0초에 짝 몽타주 노티파이가 있다. 적의 `OnInteracted`가 채우던 `TargetTags`는 읽는 곳이 없어 지웠다. `AM_Shared_BackstabFinisher`는 참조처가 없다.
- 궁극기(`b652ee043`): `CutsceneSequence` 프로퍼티를 지우고 `UWxAnimNotify_SkillCutscene`을 궁극기 몽타주 0초에 둔다. 이 노티파이는 시퀀스를 담는 표식이고 불려도 하는 일이 없다. 어빌리티가 재생 전에 읽어 기존 흐름(컷신 → 몽타주)을 그대로 쓴다. 표식이 없으면 몽타주만 재생하고, 몽타주 없이 컷신만 있는 분기는 없앴다. 몽타주 도중 컷신을 열면 컷신 컴포넌트가 시전자 몽타주를 멈춰 어빌리티가 끝나기 때문에 시점형을 택하지 않았다.

## 병합에서 확인한 엔진 규칙

- 0초가 아닌 섹션 시작에 놓인 노티파이는 앞 섹션 끝에서 불린다(`UAnimMontage::CalculateOffsetFromSections`, `AnimMontage.cpp:887`). 그래서 단계 섹션 시작에 정확히 맞춘 단발 노티파이는 그 단을 재생할 때 불리지 않는다. 병합 도구는 원본의 트리거 오프셋을 그대로 둔다.
- 몽타주 길이를 늘리면 끝에 닿은 노티파이 구간이 늘어나거나 밀린다(`AnimSequenceBase.cpp:1101`). 도구는 기존 노티파이 시각을 보존한다.
- 같은 노티파이 구간을 다른 인스턴스가 수집하면 엔진은 새 구간으로 보지 않고 활성 구간의 참조를 그 인스턴스 것으로 바꾼다(`UAnimInstance::TriggerAnimNotifies`, `AnimInstance.cpp:1890`). 1타 콤보 창의 끝이 부동소수 오차로 2타 섹션 시작보다 6e-8초 뒤에 있어 평타 2타 중 입력이 1타로 돌아간 버그가 이 때문이었다. `SnapNotifyEndsToSections`가 경계 2ms 안쪽에서 끝나는 구간의 끝을 경계에 정확히 맞춰 해결했다(콤보·패턴 8개에서 구간 20개).
- 합친 뒤 참조가 없어진 원래 몽타주 38개를 지웠다.
