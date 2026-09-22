# 전투 어빌리티와 이펙트

## 한줄 요약

전투 어빌리티와 이펙트는 공통 활성화 규칙을 따르며, 부여 구성과 수치는 데이터 에셋·테이블로 정의합니다.

## 확장 규약

- **새 어빌리티**: `UWxAbilityBase` 상속(`Public/AbilitySystem/Ability/WxAbility_*`). `AbilityDataRow`(`FWxAbilityTableRow`)에서 쿨다운·충전 수·코스트를 읽고, `EWxAbilityActivationGroup`(Independent/Exclusive/Override)과 활성화마다 Blocking에서 다시 시작하는 `EWxAbilityActionPhase`를 따른다. 코스트는 공용 `UWxEffect_Cost`, 쿨다운은 `UWxEffect_Cooldown` 파생 GE가 같은 행 수치를 쓴다. 플레이어 콤보는 `UWxAbility_Attack`/`UWxAbility_Skill`처럼 콤보 창 안의 재발동으로, AI 패턴은 `UWxAbility_Pattern`처럼 한 번의 발동이 배열 전체를 재생하는 방식으로 나뉜다.
- **새 이펙트**: `UGameplayEffect` 파생(`Public/AbilitySystem/Effect/WxEffect_*`). 수치·표시 데이터를 스펙에 싣지 않고 `UWxEffectComponent_Table`(+`FWxEffectTableRow`)을 GE에 붙이면 `UWxMMC_EffectMagnitude`/`UWxMMC_EffectDuration`이 계산 시점에 GE 정의에서 행을 조회한다. 이 컴포넌트가 `UGameplayEffectUIData` 파생인 것은 WxUI가 WxCombat을 참조할 수 없어 양쪽이 아는 엔진 클래스가 유일한 조회 앵커이기 때문이다.
- **데이터 주도**: 부여는 `UWxAbilitySet` DataAsset, 수치는 DataTable 행(`FWxAbilityTableRow`·`FWxEffectTableRow`·`FWxDamageTableRow`·`FWxCombatAttributeInitTableRow`). 한 캐릭터에 여러 세트를 꽂으면 뒤 세트의 어트리뷰트 행이 앞 세트를 덮는다. 공격 1건의 성질(계수·크리/가드/패리 허용·HitReact 태그·추가 GE)은 전부 `FWxDamageTableRow` 한 행이 쥔다.

## 진입점

1. [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h](../../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h) — 캐릭터에 무엇이 어떻게 부여되는지. 시스템 진입 구조가 여기서 잡힌다
2. [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h](../../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h) — 발동/배타/캔슬 창 모델. 콤보·후딜·선입력이 모두 이 모델 위에 얹혀 있다

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../../index.md) · [운영 절차](../../wiki-maintenance.md)

2026-09-22 기존 WxCombat 설명에서 분리했습니다. 위 검증 기준을 승계하며 코드 재검증은 아닙니다.

## 검증 범위와 근거

[Build.cs](../../../../Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs)·[descriptor](../../../../Plugins/WxCombat/WxCombat.uplugin), [ASC](../../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp)·[AbilitySet](../../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp)의 부여·입력 경로, [AbilityBase](../../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp)의 발동·종료·비용·쿨다운 경로를 확인했다. AbilitySet 자체가 권한을 검사하는 것은 아니며, 현재 캐릭터의 호출부가 서버에서 부여한다.

- [AbilityBase](../../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp): `Effect.IgnoreCosts`·`Effect.IgnoreCooldowns`가 검사와 적용을 모두 우회한다. `Effect.IgnoreAbilityTags`는 태그 조건을 우회하지만 활성화 그룹 검사까지 제거하지 않는다. `ActivationOwnedEffects`는 활성화에서 적용하고 종료 시 서버에서 제거한다.

실제 비용·쿨다운 DataTable과 BP·AbilitySet 배선, 게임 실행은 미검증입니다.

관련: [전투 개요](index.md), [피해 처리](damage.md), [전투 자원](resources.md).

*이관 원문의 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 187파일 — 원문 출처 보존; 현재 확인 범위는 이 참고 정보 참고*

</details>
