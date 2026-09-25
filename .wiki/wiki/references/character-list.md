---
title: "캐릭터 목록"
category: reference
sources:
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
created: 2026-09-25
updated: 2026-09-25
tags: [wx, combat]
aliases: ["WxAbilitySet 목록", "캐릭터 속성 초기값"]
confidence: high
volatility: warm
verified: 2026-09-25
summary: "AbilitySets를 가진 캐릭터 BP의 GAS 구성으로, 캐릭터별 WxAbilitySet과 각 WxAbilitySet이 주는 어빌리티·이펙트, 속성 초기값을 보인다."
---

# 캐릭터 목록

에디터 없이 캐릭터 BP·ABS_·DT_ 에셋을 읽어 만든 표다. [ExportAbilitySystemLists.bat](../../../BatchFiles/ExportAbilitySystemLists.bat)이 어빌리티·이펙트·캐릭터 목록을 함께 다시 만들고 [OpenWiki.bat](../../../BatchFiles/OpenWiki.bat)도 위키를 열 때 다시 만든다. 손으로 고치지 않는다.

- 에셋은 이름으로 적고 파일은 `Content/**/<이름>.uasset`로 찾는다. 같은 이름이 여러 폴더에 있으면 폴더를 앞에 붙인다.
- 캐릭터는 ASC의 `AbilitySets`를 저장한 BP이고, 이 문서는 그 GAS 구성(WxAbilitySet이 주는 어빌리티·이펙트와 속성 초기값)만 다룬다.
- 어빌리티 상세는 [어빌리티 목록](../references/ability-list.md), 이펙트 상세는 [이펙트 목록](../references/effect-list.md)에 있다.

## 캐릭터

| 캐릭터 | 부모 | AbilitySets (부여 순서) |
|---|---|---|
| BP_Doppelganger | AWxEnemyCharacter | ABS_Doppelganger |
| BP_HGTest | AWxPlayerCharacter | ABS_Shared_Player, ABS_HGTest |
| BP_Minion | AWxEnemyCharacter | ABS_Minion, ABS_Shared_Enemy |
| BP_Sandbag | AWxEnemyCharacter | ABS_Sandbag |
| BP_Soldier | AWxEnemyCharacter | ABS_Soldier, ABS_Shared_Enemy |
| Enemy/BP_Template | AWxEnemyCharacter | ABS_Shared_Enemy, Enemy/ABS_Template |
| Player/BP_Template | AWxPlayerCharacter | ABS_Shared_Player, Player/ABS_Template |

## WxAbilitySet

같은 입력의 어빌리티가 여럿이면 캐릭터의 AbilitySets 순서, WxAbilitySet 안의 부여 순서대로 시도한다.

| WxAbilitySet | 캐릭터 | AttributeInitRow | GrantedAbilities (부여 순서) | GrantedEffects |
|---|---|---|---|---|
| ABS_Doppelganger | BP_Doppelganger | DT_CharacterAttribute.Minion |  | WxEffect_IgnoreAggro, WxEffect_IgnoreCooldowns, WxEffect_IgnoreCosts, WxEffect_IgnoreAbilityActivationTags |
| ABS_HGTest | BP_HGTest | DT_CharacterAttribute.HGTest | GA_HGTest_Attack_Light_1, GA_HGTest_Attack_Light_2, GA_HGTest_Attack_Heavy_1, GA_HGTest_Attack_Heavy_2, GA_HGTest_Attack_Air, GA_HGTest_Attack_DodgeCounter, GA_HGTest_Skill_1, GA_HGTest_Skill_2, GA_HGTest_Skill_3, GA_HGTest_Ultimate_1, GA_HGTest_Ultimate_2, GA_HGTest_Passive |  |
| ABS_Minion | BP_Minion | DT_CharacterAttribute.Minion | GA_Minion_Attack_Heavy, GA_Minion_Skill_1, GA_Minion_Skill_2 | WxEffect_IgnoreAggro, WxEffect_IgnoreCooldowns, WxEffect_IgnoreCosts |
| ABS_Sandbag | BP_Sandbag | DT_CharacterAttribute.Sandbag | GA_Shared_HitReact, GA_Shared_HitReact_KnockUp, GA_Shared_HitReact_Normal, GA_Shared_Death, GA_Shared_Groggy |  |
| ABS_Soldier | BP_Soldier | DT_CharacterAttribute.TemplateEnemy | GA_Soldier_Pattern_1, GA_Soldier_Pattern_2, GA_Soldier_Pattern_3 |  |
| Enemy/ABS_Template | Enemy/BP_Template | DT_CharacterAttribute.TemplateEnemy | GA_Template_Pattern_1, GA_Template_Pattern_2 |  |
| Player/ABS_Template | Player/BP_Template | DT_CharacterAttribute.TemplatePlayer | GA_Template_Attack_Light, GA_Template_Attack_Heavy, GA_Template_Attack_Air, GA_Template_Attack_DodgeCounter, GA_Template_Skill, GA_Template_Ultimate, GA_Template_Passive |  |
| ABS_Shared_Enemy | BP_Minion, BP_Soldier, Enemy/BP_Template | DT_CharacterAttribute.TemplateEnemy | GA_Shared_HitReact, GA_Shared_HitReact_KnockUp, GA_Shared_HitReact_Normal, GA_Shared_Death, GA_Shared_Groggy |  |
| ABS_Shared_Player | BP_HGTest, Player/BP_Template | DT_CharacterAttribute.TemplatePlayer | GA_Shared_HitReact, GA_Shared_HitReact_KnockUp, GA_Shared_HitReact_Normal, GA_Shared_Death, GA_Shared_Guard, GA_Shared_Dodge, GA_Shared_GuardReact, GA_Shared_Finisher, GA_Shared_LockOn, GA_Shared_Sprint, GA_Shared_Interact, GA_Shared_UseItem | WxEffect_RegenSP |

## 속성 초기값 · DT_CharacterAttribute

| 행 | HP | MaxHP | SP | MaxSP | GP | MaxGP | MP | MaxMP | UP | MaxUP | ATK | DEF | CritRate | CritDMG | WxAbilitySet |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| TemplatePlayer | 100 | 100 | 100 | 100 | 0 | 0 | 50 | 100 | 100 | 100 | 40 | 40 | 5 | 50 | ABS_Shared_Player, Player/ABS_Template |
| TemplateEnemy | 100 | 100 | 0 | 0 | 0 | 50 | 0 | 0 | 0 | 0 | 20 | 40 | 0 | 0 | ABS_Shared_Enemy, ABS_Soldier, Enemy/ABS_Template |
| Sandbag | 10000 | 10000 | 0 | 0 | 0 | 100 | 0 | 0 | 0 | 0 | 20 | 40 | 0 | 0 | ABS_Sandbag |
| Boss | 1000 | 1000 | 0 | 0 | 0 | 200 | 0 | 0 | 0 | 0 | 20 | 40 | 0 | 0 |  |
| HGTest | 100 | 100 | 100 | 100 | 0 | 0 | 0 | 3 | 0 | 100 | 40 | 40 | 5 | 50 | ABS_HGTest |
| Minion | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 40 | 40 | 5 | 50 | ABS_Doppelganger, ABS_Minion |

## 관련 문서

- [[ability-list|어빌리티 목록]] ([어빌리티 목록](../references/ability-list.md))
- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))
- [[effect-list|이펙트 목록]] ([이펙트 목록](../references/effect-list.md))

## Sources

- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — WxAbilitySet의 부여 규칙과 데이터 배치
