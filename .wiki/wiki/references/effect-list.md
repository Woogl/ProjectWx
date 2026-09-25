---
title: "이펙트 목록"
category: reference
sources:
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
created: 2026-09-25
updated: 2026-09-25
tags: [wx, combat]
aliases: ["GE_ 목록", "GameplayEffect 목록", "DT_Damage 행 목록"]
confidence: high
volatility: warm
verified: 2026-09-25
summary: "GE_ 에셋의 모디파이어·컴포넌트 저장값, 에셋이 참조하는 C++ 이펙트·컴포넌트·계산 클래스, 피해 행(DT_Damage) 값을 에셋 사용처와 함께 보인다."
---

# 이펙트 목록

에디터 없이 GE_·DT_ 에셋과 그 참조를 읽어 만든 표다. [ExportAbilitySystemLists.bat](../../../BatchFiles/ExportAbilitySystemLists.bat)이 어빌리티·이펙트·캐릭터 목록을 함께 다시 만들고 [OpenWiki.bat](../../../BatchFiles/OpenWiki.bat)도 위키를 열 때 다시 만든다. 손으로 고치지 않는다.

- 에셋은 이름으로 적고 파일은 `Content/**/<이름>.uasset`로 찾는다. 같은 이름이 여러 폴더에 있으면 폴더를 앞에 붙인다.
- C++ 이펙트·컴포넌트·계산 클래스의 정의는 코드에서 본다. 이 문서는 에셋에 저장된 값과 에셋의 참조만 적는다.
- 에셋 사용처는 이름이 `GA_`·`ABS_`·`GE_`·`AM_`·`BT_`·`ST_`·`DA_`·`DT_`·`BP_`로 시작하는 에셋의 참조에서 찾는다. `(파생)`은 그 클래스를 부모로 둔 GE_다.

## GE_ 에셋

값은 에셋에 저장된 값이다. 빈 칸은 "없음"이 아니라 저장된 값이 없어 부모 기본값을 따른다는 뜻이고, `없음`은 부모 값을 빈 값으로 덮어쓴 것이다. 값 없이 이름만 적힌 컴포넌트도 부모 기본값을 쓴다. 부모가 C++ 클래스면 기본값은 그 클래스와 상위 클래스의 생성자·헤더 초기값에 있고, 부모 칸 링크가 그 클래스의 생성자 파일이다. 모디파이어는 `어트리뷰트 연산 크기`로 적는다.

| 이펙트 | 부모 | Modifiers | GEComponents | 기타 | 에셋 사용처 |
|---|---|---|---|---|---|
| GE_HGTest_Attack_Heavy_2 | UGameplayEffect | MP AddBase 1 |  |  | GA_HGTest_Attack_Heavy_2 |
| GE_HGTest_AddUP | UGameplayEffect | UP AddBase 10 | TargetTagRequirementsGameplayEffectComponent{ApplicationTagRequirements=Master.Doppelganger} |  | GA_HGTest_Passive |
| GE_HGTest_Skill_2 | UGameplayEffect | MP AddBase 1 |  |  | GA_HGTest_Skill_2 |
| GE_HGTest_Skill_3 | UGameplayEffect | MP AddBase 1 |  |  | GA_HGTest_Skill_3 |
| GE_Template_Passive_AddUP | UGameplayEffect | UP AddBase 5 |  |  | GA_Template_Passive |
| GE_Shared_GuardReduction | [UWxEffect_GuardReduction](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_GuardReduction.cpp) | GuardReductionScale AddBase 0.5 | TargetTagsGameplayEffectComponent; AssetTagsGameplayEffectComponent; WxEffectComponent_UIData{Icon=T_UI_Shield} |  | GA_Shared_Guard |

## C++ 클래스의 에셋 사용처

`UGameplayEffect`·`UGameplayEffectComponent`·`UGameplayEffectUIData`·`UGameplayModMagnitudeCalculation`·`UGameplayEffectExecutionCalculation`을 상속한 프로젝트 C++ 클래스 중 에셋이 참조하는 것만 적는다.

| 클래스 | 에셋 사용처 |
|---|---|
| UWxEffectComponent_UIData | GE_Shared_GuardReduction |
| UWxEffect_FullHP | ST_CheckPoint |
| UWxEffect_GuardReduction | GE_Shared_GuardReduction (파생) |
| UWxEffect_HealPercent | DA_Potion |
| UWxEffect_IgnoreAbilityTags | ABS_Doppelganger |
| UWxEffect_IgnoreAggro | ABS_Doppelganger, ABS_Minion |
| UWxEffect_IgnoreCooldowns | ABS_Doppelganger, ABS_Minion |
| UWxEffect_IgnoreCosts | ABS_Doppelganger, ABS_Minion |
| UWxEffect_Invincible | AM_Shared_Dodge |
| UWxEffect_MoveSpeedOverride | BT_Doppelganger |
| UWxEffect_MoveSpeedScale | BT_Soldier, BT_Template |
| UWxEffect_PerfectGuard | AM_Shared_Guard, AM_Shared_GuardReact |
| UWxEffect_RegenSP | ABS_Shared_Player |

## 피해 행 · DT_Damage

`UWxEffect_Damage`를 만들 때 쓰는 행이다. 사용처는 그 행을 가리키는 에셋(엔진의 행 참조 기록)이다.

| 행 | CoeffATK | HitReactTag | bCanCritical | bCanGuard | bCanParry | AdditionalEffects | 사용처 |
|---|---|---|---|---|---|---|---|
| AM_Template_Attack_L | 1 | HitReact.Normal | true | true | true |  | AM_Template_Attack_Light |
| AM_Template_Attack_LL | 1 | HitReact.Normal | true | true | true |  | AM_Template_Attack_Light |
| AM_Template_Attack_LLL | 1 | HitReact.Normal | true | true | true |  | AM_Template_Attack_Light |
| AM_Template_Attack_LLLL | 1.2 | HitReact.Normal | true | true | true |  | AM_Template_Attack_Light |
| AM_Template_Attack_H | 1 | HitReact.Normal | true | true | true |  | AM_Template_Attack_H |
| AM_Template_Attack_Air | 2 | HitReact.Normal | true | true | true |  | AM_Template_Attack_L_Air |
| AM_Template_Attack_DodgeCounter_1 | 1 | HitReact.Normal | true | true | true |  | AM_Template_Attack_DodgeCounter |
| AM_Template_Attack_DodgeCounter_2 | 1 | HitReact.Normal | true | true | true |  | AM_Template_Attack_DodgeCounter |
| AM_Template_Skill | 2 | HitReact.KnockUp | true | true | true |  | AM_Template_Skill |
| AM_Template_Ultimate | 3 | HitReact.KnockBack | true | false | false |  | AM_Template_Ultimate |
| AM_Shared_Finisher | 3 |  | false | false | false |  | AM_Shared_Finisher |
| AM_Template_Pattern_1_1 | 1 | HitReact.Normal | true | true | false |  | AM_Soldier_Pattern_1, AM_Soldier_Pattern_2, AM_Soldier_Pattern_3, AM_Template_Pattern_1 |
| AM_Template_Pattern_1_2 | 1 | HitReact.Normal | true | true | false |  | AM_Soldier_Pattern_1, AM_Soldier_Pattern_2, AM_Template_Pattern_1 |
| AM_Template_Pattern_1_3 | 1 | HitReact.Normal | true | true | false |  | AM_Soldier_Pattern_1, AM_Template_Pattern_1 |
| AM_Template_Pattern_1_4 | 1 | HitReact.Normal | true | true | true |  | AM_Template_Pattern_1 |
| BP_Template_Projectile | 0.5 | HitReact.Normal | true | true | false |  | BP_Template_Projectile |
| AM_Minion_Skill_1 | 1 | HitReact.Normal | true | true | true |  | AM_Minion_Skill_1 |
| AM_Minion_Skill_2 | 1 | HitReact.Normal | true | true | true |  | AM_Minion_Skill_2 |
| AM_HGTest_Attack_Light_1 | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Attack_Light_1 |
| AM_HGTest_Attack_Light_2 | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Attack_Light_2 |
| AM_HGTest_Attack_Heavy | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Attack_Heavy_1, AM_HGTest_Attack_Heavy_2 |
| AM_HGTest_Attack_Air | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Attack_L_Air |
| AM_HGTest_Attack_DodgeCounter | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Attack_DodgeCounter |
| AM_HGTest_Ultimate_1 | 0.5 | HitReact.Normal | true | true | true |  | AM_HGTest_Ultimate_1 |
| AM_HGTest_Ultimate_2 | 3 | HitReact.KnockDown | true | true | true |  | AM_HGTest_Ultimate_2 |
| AM_HGTest_Skill_2 | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Skill_2 |
| AM_HGTest_Skill_3 | 1 | HitReact.Normal | true | true | true |  | AM_HGTest_Skill_3 |

표에 없는 행을 가리키는 참조: `AM_Attack_LLLL` ← AM_HGTest_Attack_DodgeCounter; `AM_Attack_H` ← AM_Minion_Attack_Heavy; `BP_Projectile` ← BP_Soldier_Projectile

## 관련 문서

- [[ability-list|어빌리티 목록]] ([어빌리티 목록](../references/ability-list.md))
- [[character-list|캐릭터 목록]] ([캐릭터 목록](../references/character-list.md))
- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))

## Sources

- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — GE_가 가진 데이터와 배치 규칙
