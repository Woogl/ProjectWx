---
title: "WxCombat 정리: 구간 GE 노티파이 자기 핸들 제거·처형 피해 어빌리티 직접 적용·퍼펙트 가드 Cue 통합"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, combat, damage, finisher, architecture]
summary: "구간 GE 노티파이가 몽타주 인스턴스별로 자기가 건 핸들의 스택 하나만 걷게 했고, 처형 피해는 UWxFinisherDamageComponent를 없애고 UWxAbility_Finisher가 서버에서 직접 적용한다. 퍼펙트 가드 Cue는 UWxCueNotify_Hit를 부모로 쓰고 전용 클래스를 지웠다. ExecCalc 캡처 정의 통합은 동작 변화가 없다."
---

# WxCombat 정리 네 건

2026-09-24 커밋 `ce6295184`·`4c1bf3e52`·`74fc58853`·`ca84c9aac`. HEAD `ca84c9aac`에서 코드를 다시 읽어 대조했다.

## 구간 GE 노티파이는 자기가 건 효과만 걷는다 (`ce6295184`)

- 문제: `UWxAnimNotifyState_ApplyGameplayEffect`가 끝에서 `RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1)`로 같은 클래스의 GE를 전부 한 스택씩 걷었다. 극한 회피·처형·컷신 무적, 가드→가드히트 퍼펙트가드처럼 같은 GE를 건 구간이 겹치면 다른 구간의 효과까지 함께 사라졌다. 이를 피하려면 GE를 스택형으로 만들어야 했다.
- 사용자 결정: "스택형이 아닌 GE도 쓸 수 있게" 하고 싶다며 ANS가 자기 핸들만 제거하는 B안을 골랐다("네 B로 합시다"). 결정 경위는 [Nameplate 작업 자료](../../../.agents/workflow/tasks/nameplate-manager.md)의 코드 리뷰 후속 절에 있다.
- 구조
  - 노티파이 객체는 몽타주 에셋에 하나라 여러 캐릭터가 공유한다. 그래서 구간은 몽타주 인스턴스 ID로 가른다. `TMap<int32, FActiveGameplayEffectHandle> AppliedEffects`에 구간마다 건 핸들을 보관하고(서버에서만 채워진다), 끝에서 그 핸들의 스택 하나만 뺀다(`RemoveActiveGameplayEffect(Handle, 1)`).
  - 몽타주 인스턴스 ID는 전역 카운터라 고유하다(`AnimMontage.cpp:1686`).
  - 큐 경로는 이벤트 참조의 `FAnimNotifyMontageInstanceContext`에서, 브랜칭 포인트 경로는 `BranchingPointNotifyBegin/End` 오버라이드로 페이로드의 `MontageInstanceID`에서 받는다. 엔진 기본 브랜칭 구현은 빈 이벤트 참조로 `NotifyBegin/End`를 불러 몽타주 인스턴스를 잃는다(`AnimNotifyState.cpp:46`). 브랜칭 노티파이는 큐에서 걸러지므로 두 경로가 겹치지 않는다(`UAnimMontage::FilterOutNotifyBranchingPoints`).
  - 몽타주가 아닌 재생에서는 끝에서 걷을 구간을 가를 수 없어 경고를 남기고 적용하지 않는다.
  - `UWxCombatLibrary::ApplyEffect`는 적용한 핸들을 반환한다(호출부는 이 ANS 하나).
  - `UWxAbilityBase::EndAbility`의 `ActivationOwnedEffects` 제거와 `UWxSkillCutsceneComponent::Finish`의 컷신 무적 제거도 핸들의 스택 하나만 뺀다. 스택형 GE는 다른 소유자의 적용과 한 핸들로 합쳐지기 때문이다. 비스택형이면 전체 제거와 같다.
  - `UWxEffect_Invincible`·`UWxEffect_PerfectGuard`의 스택 설정은 바꾸지 않았다.
- 검증: 임시 자동화 테스트 `Wx.Combat.ApplyEffectNotify.OwnHandleOnly` 통과(비스택형 GE 두 구간 겹침, 핸들 소유자와의 겹침, 브랜칭 경로 겹침, 몽타주가 아닌 재생의 미적용). 사용자 지침에 따라 테스트 파일은 삭제하고 다시 빌드해 통과를 확인했다. 플레이는 미검증.

## 처형 피해는 처형 어빌리티가 직접 적용한다 (`4c1bf3e52`)

- 문제: `UWxFinisherDamageComponent`(플레이어 캐릭터 네이티브 컴포넌트)가 어빌리티가 이미 쥔 대상과 피해 행을 `BeginFinisherDamage`로 복사해 들고, 어빌리티 종료 시 `EndFinisherDamage`로 수명을 맞췄다.
- 현재: `UWxAbility_Finisher`가 서버에서만 `UAbilityTask_WaitGameplayEvent`로 `Event.ApplyFinisherDamage`를 한 번(OnlyTriggerOnce) 기다린다. 받으면 발동 순간의 대상 태그로 고른 변형(`bBackstab`)의 `DamageDataRow`로 `ApplyDamage(아바타, 대상, 행, 대상 위치 HitResult)`를 부른다. 대상 상태는 이후 바뀌므로 피해 시점에 변형을 다시 고르지 않는다. 어빌리티가 끝나면 태스크와 함께 대기도 끝난다.
- `UWxAnimNotify_FinisherDamage`는 이벤트만 보낸다. 페이로드의 `OptionalObject`(노티파이)·`OptionalObject2`(메시)는 컴포넌트가 출처 확인에 쓰던 것이라 지웠다.
- 플레이어 캐릭터의 네이티브 컴포넌트를 지웠고, `BP_Template`(Player)·`BP_HGTest`를 사라진 컴포넌트의 LogLinker 경고를 없애려 다시 저장했다.
- 인게임 처형 피해는 미검증.

## 퍼펙트 가드 Cue는 Hit Cue 클래스를 쓴다 (`ca84c9aac`)

- `UWxCueNotify_PerfectGuard`는 `UWxCueNotify_Hit`의 Niagara·사운드 재생을 그대로 복제하고 있었다. `GC_PerfectGuard`의 부모를 `UWxCueNotify_Hit`로 바꾸고 `GameplayCueTag`를 `GameplayCue.PerfectGuard`로 지정한 뒤 전용 클래스를 지웠다.
- 발행은 그대로다. `GameplayCue.Hit`는 `_DamageReaction`이, `GameplayCue.PerfectGuard`는 `_PerfectGuard` 컴포넌트가 서버에서 발행한다.
- `UWxCueNotify_Hit`에는 `CameraShake` 속성도 있어(각 머신이 자기 로컬 공격자 또는 피격자 컨트롤러만 흔든다) 퍼펙트 가드 Cue도 이제 이를 지정할 수 있다. `GC_PerfectGuard`의 실제 값은 확인하지 않았다.

## ExecCalc 캡처 정의 통합 (`74fc58853`)

`FWxDamageBaseStatics`·`FWxDamageExecutionStatics`로 나뉜 속성 캡처 정의는 사라진 기본 피해 MMC 기준이었다. `UWxExecCalc_Damage` 하나만 쓰므로 `FWxDamageStatics` 하나와 접근자 `GetDamageStatics()` 하나로 합쳤다. 계산·판정 동작은 바뀌지 않는다.

근거: [구간 GE 노티파이](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp), [CombatLibrary](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp), [AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp), [컷신](../../../Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp), [처형 어빌리티](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp), [처형 피해 노티파이](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherDamage.cpp), [Hit Cue](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_Hit.cpp), [피해 GE·ExecCalc](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp).
