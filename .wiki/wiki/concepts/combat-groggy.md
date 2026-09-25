---
title: "그로기"
category: concept
sources:
  - "raw/notes/2026-09-22-current-groggy.md"
  - "raw/notes/2026-09-22-current-damage.md"
  - "raw/notes/2026-09-22-groggy-montage-stop-fix.md"
  - "raw/notes/2026-09-24-ai-brain-control-single-owner.md"
  - "raw/notes/2026-09-25-ability-montage-section-model.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, groggy]
aliases: []
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "GP 상한은 그로기 이벤트를 발생시키며, 실제 유지·종료는 어빌리티와 서버의 GP 감소 경로가 맡는다."
---

# 그로기

GP 상한은 그로기 이벤트를 발생시키며, 실제 유지·종료는 어빌리티와 서버의 GP 감소 경로가 맡는다.

## 기획과 구현

[기획 초안](../../../Docs/CombatDesign/그로기_시스템.md)은 최대 스태거에서 그로기에 진입하고 이동·공격을 막으며 받는 피해를 30% 증가시키고, 게이지가 0이 되면 종료한다고 설명한다. 이 요구 전체가 현재 에셋에 반영되었다고 판정하지 않는다. 특히 공통 피해 계산의 그로기 분기는 GP 누적 억제이며, 그 분기만으로 30% 증가를 구현했다고 볼 수 없다.

`PostGameplayEffectExecute`는 MaxGP > 0, GP >= MaxGP, `Ability.Groggy` 부재일 때 `Event.Groggy`를 보낸다. 이벤트 발행과 어빌리티 활성화 성공은 구분한다. 사망 태그는 어빌리티를 차단하고, 몽타주 미지정·커밋 실패·ASC 부재도 활성화 종료 경로다.

## 수명과 정리

```mermaid
stateDiagram-v2
  [*] --> Request: GP 상한·태그 조건
  Request --> Active: 부여된 어빌리티의 활성화 검사 통과
  Request --> End: 몽타주·커밋 등 실패
  Active --> End: 서버 GP가 거의 0 또는 음수
  Active --> End: 사망 감지 등 종료
  End --> [*]: 몽타주 정지, 감소 효과·구독·폴링 정리, 태그 제거
```

그로기는 ServerInitiated/ServerOnly 보안 정책을 사용한다. 서버가 GP 변경을 구독하고 몽타주 길이를 감소 효과의 Duration으로 잠가 설정한다. 0.1초 폴링으로 몽타주 상태를 확인한다. 다른 몽타주가 있으면 그로기 몽타주를 덮지 않고, 사망 태그를 확인하면 종료한다.

`EndAbility`는 폴링·GP 구독·감소 GE를 해제한다. 그로기 몽타주는 ASC의 현재 몽타주인지와 관계없이 AnimInstance에서 직접 정지한다. 가산 슬롯 피격 몽타주가 ASC의 현재 몽타주 자리를 차지하면 `StopMontageIfCurrent`로는 그 아래에서 루프 중인 그로기 몽타주를 멈추지 못했기 때문이다(커밋 `c2b1088a0`). [그로기 어빌리티](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp)의 수명과 실제 AbilitySet·몽타주를 함께 확인해야 적별 동작을 판단할 수 있다.

## 그로기 중 피격

대상이 그로기면 넉 계열 반응 태그(`HitReact.KnockBack`·`KnockDown`·`KnockUp`)를 [`UWxEffectComponent_DamageReaction`](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageReaction.cpp)이 `HitReact.Normal`로 낮춰 피격 이벤트에 싣는다. 긴 넉 몽타주가 그로기 몽타주를 밀어내는 동안에도 GP 감소가 돌아 그로기 창이 잘리기 때문이다. 코드 주석에 따르면 GP 반영이 이 컴포넌트보다 먼저라 그로기를 시작시킨 타격도 낮춘다. 이 판단은 2026-09-24에 `UWxAbility_HitReact`에서 옮겼다(커밋 `15bcc1682`). 반응 이벤트 전체 흐름은 [피해 처리와 전투 연출](combat-damage.md)에 있다.

## AI 트리 잠금

그로기 어빌리티는 AI 트리를 건드리지 않는다. `AWxAIController`가 `Ability.Groggy` 태그가 붙으면 트리를 `EAIRequestPriority::Reaction`으로 잠그고, 태그가 빠지면 푼다. 트리를 멈추지 않고 잠그므로 그로기가 끝나면 멈춘 자리에서 이어 간다. 사망으로 트리가 정지된 뒤에도 태그가 빠질 때 잠금을 푼다. 엔진이 트리를 다시 시작할 때 일시정지를 초기화하지 않기 때문이다. 이미 진행 중인 MoveTo 경로 추종과 포커스 회전은 잠금으로 멈추지 않는다([AI 컨트롤러](../../../Source/WxGame/Controller/WxAIController.cpp), [WxAI](../topics/ai.md)).

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-damage|피해 처리와 전투 연출]] ([피해 처리와 전투 연출](../concepts/combat-damage.md))
- [[combat-finisher|그로기 피니시와 뒤잡]] ([그로기 피니시와 뒤잡](../concepts/combat-finisher.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-groggy.md)
- [근거 2](../../raw/notes/2026-09-22-current-damage.md)
- [그로기 종료 시 몽타주 정지 경로 수정 조사](../../raw/notes/2026-09-22-groggy-montage-stop-fix.md) — 가산 피격 중 몽타주 정지 수정
- [AI 트리 정지·잠금 컨트롤러 단독화](../../raw/notes/2026-09-24-ai-brain-control-single-owner.md) — 그로기 AI 잠금 주체 이동
- [어빌리티 규칙 변경과 몽타주 섹션 모델](../../raw/notes/2026-09-25-ability-montage-section-model.md) — 그로기 중 넉 계열 강등을 대미지 반응 컴포넌트로 이동

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 그로기 몽타주 정지 경로는 HEAD `60c324c714b1dab10cd48d36cabad63ace232716`(해당 파일 마지막 변경 `c2b1088a0`) 기준으로 갱신했다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

2026-09-24: AI 트리 잠금 주체를 그로기 어빌리티에서 `AWxAIController`로 옮긴 작업 트리 코드와 대조해 수명 도식·본문을 고치고 "AI 트리 잠금" 절을 추가했다. 빌드 통과, 사용자 인게임 확인.

2026-09-25: 그로기 중 넉 계열 강등 위치(커밋 `15bcc1682`)를 HEAD `d63ce0630`의 `UWxEffectComponent_DamageReaction`·`UWxAbility_HitReact` 코드와 대조해 "그로기 중 피격" 절을 추가했고, 나머지 절의 `UWxAbility_Groggy`·`UWxCombatAttributeSet`·`AWxAIController` 서술도 같은 HEAD에서 다시 확인했으며, 강등의 인게임 동작은 AI가 확인하지 않았다(원자료의 빌드 통과와 전반적인 사용자 플레이 확인만 있다).

</details>
