---
title: "전투 어빌리티와 이펙트"
category: concept
sources:
  - "raw/notes/2026-09-22-current-combat.md"
  - "raw/notes/2026-09-22-current-ability-cost-cooldown.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, combat]
aliases: ["GAS"]
confidence: medium
volatility: warm
verified: 2026-09-22
summary: "발동 그룹과 액션 단계는 서로 다른 계약이며, 비용·쿨다운·표시 데이터는 어빌리티 행으로 연결한다."
---

# 전투 어빌리티와 이펙트

발동 그룹과 액션 단계는 서로 다른 계약이며, 비용·쿨다운·표시 데이터는 어빌리티 행으로 연결한다.

## 발동과 취소

`UWxAbilityBase`는 InstancedPerActor와 LocalPredicted를 기본값으로 둔다. `Independent`는 배타 점유에 참여하지 않고, `Exclusive`는 다른 배타 액션을 막으며, `Override`는 반응·사망 등 강제 행동에 사용한다. `CancelAbilitiesWithTag`로 지목한 점유자에 대한 우선 규칙도 있으므로 그룹 이름만으로 발동 결과를 결정하면 안 된다.

`Exclusive`의 실행 단계는 `Blocking`, `ComboWindow`, `Recovery`다. 콤보 창은 자기 재발동을 허용하고, Recovery는 다른 배타 액션이 끊고 들어올 수 있게 한다. `OpenComboWindow`와 `StartRecovery`는 입력 버퍼를 다시 처리한다. `CloseComboWindow`는 이미 Recovery로 넘어간 동작을 Blocking으로 되돌리지 않는다.

## 데이터와 수명

- `AbilityDataRow`: 쿨다운 시간·충전 수·코스트 종류/양·제목·설명·아이콘의 연결점이다. 비용 종류는 Custom/SP/MP/UP다.
- 비용 GE `UWxEffect_Cost`는 공용 Instant 효과다. MP·UP·SP에 Additive 모디파이어를 하나씩 두고, 각 MMC가 소스 어빌리티의 `AbilityDataRow`를 읽어 행이 고른 자원에만 `-CostAmount`를 낸다. 엔진 순정 CheckCost·ApplyCost를 그대로 쓰며, 순정 자원 부족 판정이 Additive 모디파이어만 보기 때문에 Additive를 쓴다.
- 쿨다운 GE는 `UWxEffect_Cooldown` 파생 클래스를 어빌리티별로 지정하고, 파생 클래스가 부여하는 `WxGameplayTags::Cooldown_*` 태그가 순정 쿨다운 API의 식별자다. 엔진이 스택을 GE 클래스 단위로 병합하므로 공용 쿨다운 클래스를 함께 쓰면 서로 다른 능력의 충전이 섞인다.
- 소모한 충전 하나가 쿨다운 스택 하나다. 진행 중인 회복은 새 스택으로 갱신되지 않고, 만료마다 스택 하나만 돌려주며 다음 회복을 시작하므로 충전은 직렬로 돌아온다. 충전 상한은 GE가 아니라 테이블의 MaxRecharges를 `UWxAbilityBase::CheckCooldown`이 판정한다. 지속시간은 소스 어빌리티의 쿨다운 시간에서 읽으며, 어빌리티를 거치지 않고 적용되면 경고와 함께 즉시 만료된다.
- `ActivationOwnedEffects`는 발동 수명에 묶는 효과 목록이다. 지속시간이 별도인 효과를 무조건 이 목록으로 옮기지 않는다.
- `AbilitySet`은 최대값→현재값 초기화, 효과 적용, 어빌리티 부여 순서로 구성된다. 실제 부여 목록과 데이터 행은 에셋 내부 확인이 필요하다.

## 변경 시 확인

[AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp)의 활성화 조건·종료 정리와 [AbilityTableRow](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityTableRow.h)를 함께 본다. 비용·충전 규칙은 [비용 GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp)와 [쿨다운 GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp)에서 본다. 애니메이션의 콤보·후딜 노티파이 시점과 네트워크 예측까지 이번 정적 조사로 검증한 것은 아니다.

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-damage|피해 처리와 전투 연출]] ([피해 처리와 전투 연출](../concepts/combat-damage.md))
- [[combat-resources|전투 자원과 현광의 예외]] ([전투 자원과 현광의 예외](../concepts/combat-resources.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-combat.md)
- [어빌리티 비용·쿨다운 GE 정적 조사](../../raw/notes/2026-09-22-current-ability-cost-cooldown.md) — 비용·쿨다운 GE 구조

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 비용·쿨다운 GE 설명은 HEAD `60c324c714b1dab10cd48d36cabad63ace232716` 기준으로 보강했다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
