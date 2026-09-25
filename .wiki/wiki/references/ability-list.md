---
title: "어빌리티 목록"
category: reference
sources:
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
created: 2026-09-25
updated: 2026-09-25
tags: [wx, combat]
aliases: ["GA_ 목록", "몽타주 노티파이 목록"]
confidence: high
volatility: warm
verified: 2026-09-25
summary: "GA_·AM_ 에셋에서 생성한 표로, 어빌리티마다 타입·WxAbilitySet·입력·몽타주·태그·쿨다운·비용과 몽타주 섹션·노티파이를 보인다."
---

# 어빌리티 목록

에디터 없이 GA_·AM_ 에셋을 읽어 만든 표다. [ExportAbilitySystemLists.bat](../../../BatchFiles/ExportAbilitySystemLists.bat)이 어빌리티·이펙트·캐릭터 목록을 함께 다시 만들고 [OpenWiki.bat](../../../BatchFiles/OpenWiki.bat)도 위키를 열 때 다시 만든다. 손으로 고치지 않는다.

- 에셋은 이름으로 적고 파일은 `Content/**/<이름>.uasset`로 찾는다. 같은 이름이 여러 폴더에 있으면 폴더를 앞에 붙인다.
- 값은 에셋에 저장된 값이다. 빈 칸은 "없음"이 아니라 저장된 값이 없어 부모 기본값을 따른다는 뜻이다. 기본값은 타입 칸의 C++ 클래스와 그 상위 클래스의 생성자·헤더 초기값에 있다.
- 발동 조건의 Required·Blocked는 `ActivationRequiredTags`·`ActivationBlockedTags`다. `ActivationOwnedTags`는 AbilityTags와 같으면 적지 않는다.
- WxAbilitySet 칸의 에셋을 받는 캐릭터와 그 구성은 [캐릭터 목록](../references/character-list.md)에 있다.

## 어빌리티

### Content/Character/HGTest

| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |
|---|---|---|---|---|---|---|---|---|---|---|
| GA_HGTest_Attack_Air | UWxAbility_Attack_Air | ABS_HGTest | IA_Attack_Light | AM_HGTest_Attack_L_Air |  |  |  |  |  | Icon: T_HG_Attack_01 |
| GA_HGTest_Attack_DodgeCounter | UWxAbility_Attack_DodgeCounter | ABS_HGTest | IA_Attack_Light | AM_HGTest_Attack_DodgeCounter |  |  |  |  |  | Icon: T_HG_Attack_01 |
| GA_HGTest_Attack_Heavy_1 | UWxAbility_Attack_Heavy | ABS_HGTest | IA_Attack_Heavy | AM_HGTest_Attack_Heavy_1 |  | Blocked: Movement.InAir, Ability.Dodge, Master.Minion |  | SP 20 |  | Icon: T_HG_Attack_01 |
| GA_HGTest_Attack_Heavy_2 | UWxAbility_Attack_Heavy | ABS_HGTest | IA_Attack_Heavy | AM_HGTest_Attack_Heavy_2 |  | Required: Master.Minion |  |  |  | ActivationOwnedEffects: GE_HGTest_Attack_Heavy_2; Icon: T_HG_Attack_01 |
| GA_HGTest_Attack_Light_1 | UWxAbility_Attack_Light | ABS_HGTest | IA_Attack_Light | AM_HGTest_Attack_Light_1 |  | Blocked: Movement.InAir, Ability.Dodge, Master.Doppelganger |  |  |  | Icon: T_HG_Attack_01 |
| GA_HGTest_Attack_Light_2 | UWxAbility_Attack_Light | ABS_HGTest | IA_Attack_Light | AM_HGTest_Attack_Light_2 |  | Required: Master.Doppelganger; Blocked: Movement.InAir, Ability.Dodge, Master.Minion |  |  |  | Icon: T_HG_Attack_01 |
| GA_HGTest_Passive | UWxAbility_Passive | ABS_HGTest |  |  |  |  |  |  |  | TriggeredEffects: GE_HGTest_AddUP; AbilityTriggers: {TriggerTag=Event.DamageDealt, TriggerSource=GameplayEvent} |
| GA_HGTest_Skill_1 | UWxAbility_Skill | ABS_HGTest | IA_Skill | AM_HGTest_Skill_1 | Ability.Skill, Ability.Skill.1 | Blocked: Master.Minion, Master.Doppelganger | Cooldown.Skill.1, 5초 |  | 분신 강공 협공 | Icon: T_HG_SkillE_01 |
| GA_HGTest_Skill_2 | UWxAbility_Skill | ABS_HGTest | IA_Skill | AM_HGTest_Skill_2 | Ability.Skill, Ability.Skill.2 | Required: Master.Minion |  |  | 분신 스킬 협공 | ActivationOwnedEffects: GE_HGTest_Skill_2; Icon: T_HG_SkillE_02 |
| GA_HGTest_Skill_3 | UWxAbility_Skill | ABS_HGTest | IA_Skill | AM_HGTest_Skill_3 | Ability.Skill, Ability.Skill.3 | Required: Master.Doppelganger; Blocked: Master.Minion | Cooldown.Skill.3, 5초 |  | 궁극기 중 스킬 협공 | ActivationOwnedEffects: GE_HGTest_Skill_3; Icon: T_HG_SkillE_03 |
| GA_HGTest_Ultimate_1 | UWxAbility_Ultimate | ABS_HGTest | IA_Ultimate | AM_HGTest_Ultimate_1 |  | Blocked: Master.Doppelganger | Cooldown.Ultimate, 5초 | MP 3 | 궁극기1 | Icon: T_HG_SkillR_01 |
| GA_HGTest_Ultimate_2 | UWxAbility_Ultimate | ABS_HGTest | IA_Ultimate | AM_HGTest_Ultimate_2 |  | Required: Master.Doppelganger; Blocked: Master.Minion | Cooldown.Ultimate, 5초 | UP 100 | 궁극기2 | Icon: T_HG_SkillR_02 |

### Content/Character/Minion

| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |
|---|---|---|---|---|---|---|---|---|---|---|
| GA_Minion_Attack_Heavy | UWxAbility_Attack_Heavy | ABS_Minion |  | AM_HGTest_Attack_Heavy_1 |  |  |  |  | 분신 넓은 범위 공격 후 소멸 |  |
| GA_Minion_Skill_1 | UWxAbility_Skill | ABS_Minion |  | AM_Minion_Skill_1 | Ability.Skill, Ability.Skill.1 |  |  |  | 분신 돌진 |  |
| GA_Minion_Skill_2 | UWxAbility_Skill | ABS_Minion |  | AM_Minion_Skill_2 | Ability.Skill, Ability.Skill.2 |  |  |  | 분신 돌진 후 소멸 |  |

### Content/Character/Soldier

| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |
|---|---|---|---|---|---|---|---|---|---|---|
| GA_Soldier_Pattern_1 | UWxAbility_Pattern | ABS_Soldier |  | AM_Soldier_Pattern_1 | Ability.Pattern, Ability.Pattern.1 |  |  |  |  |  |
| GA_Soldier_Pattern_2 | UWxAbility_Pattern | ABS_Soldier |  | AM_Soldier_Pattern_2 | Ability.Pattern, Ability.Pattern.2 |  |  |  |  |  |
| GA_Soldier_Pattern_3 | UWxAbility_Pattern | ABS_Soldier |  | AM_Soldier_Pattern_3 | Ability.Pattern, Ability.Pattern.3 |  |  |  |  |  |

### Content/Character/Template/Enemy

| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |
|---|---|---|---|---|---|---|---|---|---|---|
| GA_Template_Pattern_1 | UWxAbility_Pattern | Enemy/ABS_Template |  | AM_Template_Pattern_1 | Ability.Pattern, Ability.Pattern.1 |  |  |  |  |  |
| GA_Template_Pattern_2 | UWxAbility_Pattern | Enemy/ABS_Template |  | AM_Template_Pattern_2 | Ability.Pattern, Ability.Pattern.2 |  |  |  |  |  |

### Content/Character/Template/Player

| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |
|---|---|---|---|---|---|---|---|---|---|---|
| GA_Template_Attack_Air | UWxAbility_Attack_Air | Player/ABS_Template | IA_Attack_Light | AM_Template_Attack_L_Air |  |  |  |  |  | Icon: T_UI_Sword |
| GA_Template_Attack_DodgeCounter | UWxAbility_Attack_DodgeCounter | Player/ABS_Template | IA_Attack_Light | AM_Template_Attack_DodgeCounter |  |  |  |  |  | Icon: T_UI_Sword |
| GA_Template_Attack_Heavy | UWxAbility_Attack_Heavy | Player/ABS_Template | IA_Attack_Heavy | AM_Template_Attack_H |  |  |  | SP 20 |  | Icon: T_UI_Sword |
| GA_Template_Attack_Light | UWxAbility_Attack_Light | Player/ABS_Template | IA_Attack_Light | AM_Template_Attack_Light |  |  |  |  |  | Icon: T_UI_Sword |
| GA_Template_Passive | UWxAbility_Passive | Player/ABS_Template |  |  |  |  |  |  |  | TriggeredEffects: GE_Template_Passive_AddUP; AbilityTriggers: {TriggerTag=Event.DamageDealt, TriggerSource=GameplayEvent} |
| GA_Template_Skill | UWxAbility_Skill | Player/ABS_Template | IA_Skill | AM_Template_Skill |  |  | Cooldown.Skill.1, 5초 | MP 10 |  | Icon: MI_UI_Slot_1 |
| GA_Template_Ultimate | UWxAbility_Ultimate | Player/ABS_Template | IA_Ultimate | AM_Template_Ultimate |  |  | Cooldown.Ultimate, 5초 | UP 100 |  | Icon: Wx_192 |

### Content/Character/Template/Shared

| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |
|---|---|---|---|---|---|---|---|---|---|---|
| GA_Shared_Death | UWxAbility_Death | ABS_Sandbag, ABS_Shared_Enemy, ABS_Shared_Player |  |  |  |  |  |  |  |  |
| GA_Shared_Dodge | UWxAbility_Dodge | ABS_Shared_Player | IA_Dodge | AM_Shared_Dodge |  |  | Cooldown.Dodge, 2초, 충전 2 | SP 20 |  | Icon: T_UI_Sword |
| GA_Shared_Finisher | UWxAbility_Finisher | ABS_Shared_Player |  | AM_Shared_Finisher |  |  |  |  |  |  |
| GA_Shared_Groggy | UWxAbility_Groggy | ABS_Sandbag, ABS_Shared_Enemy |  | AM_Shared_Groggy |  |  |  |  |  |  |
| GA_Shared_Guard | UWxAbility_Guard | ABS_Shared_Player | IA_Guard | AM_Shared_Guard |  |  |  |  |  | ActivationOwnedEffects: GE_Shared_GuardReduction |
| GA_Shared_GuardReact | UWxAbility_GuardReact | ABS_Shared_Player |  | AM_Shared_GuardReact |  |  |  |  |  |  |
| GA_Shared_HitReact | UWxAbility_HitReact | ABS_Sandbag, ABS_Shared_Enemy, ABS_Shared_Player |  | AM_Shared_HitReact_Knock |  |  |  |  |  |  |
| GA_Shared_HitReact_KnockUp | UWxAbility_HitReact | ABS_Sandbag, ABS_Shared_Enemy, ABS_Shared_Player |  | AM_Shared_HitReact_Knockup |  |  |  |  |  |  |
| GA_Shared_HitReact_Normal | UWxAbility_HitReact | ABS_Sandbag, ABS_Shared_Enemy, ABS_Shared_Player |  | AM_Shared_HitReact |  |  |  |  |  |  |
| GA_Shared_Interact | UWxAbility_Interact | ABS_Shared_Player |  |  |  |  |  |  |  |  |
| GA_Shared_LockOn | UWxAbility_LockOn | ABS_Shared_Player | IA_LockOn |  |  |  |  |  |  | TargetingPreset: TP_LockOn |
| GA_Shared_Sprint | UWxAbility_Sprint | ABS_Shared_Player | IA_Sprint |  |  |  |  |  |  |  |
| GA_Shared_UseItem | UWxAbility_UseItem | ABS_Shared_Player | IA_UseItem | AM_Shared_UseItem |  |  |  |  |  |  |

## 몽타주

GA_가 쓰는 몽타주와 그 몽타주가 참조하는 몽타주다. 섹션은 시작 시각 순이다. 노티파이는 클래스별로 묶어 처음 나오는 시각 순으로 적고 `×N`은 개수다. `{}` 안은 그 클래스 인스턴스들에 저장된 값을 필드별로 중복 없이 모은 것이라, 어느 인스턴스·섹션의 값인지와 모든 인스턴스가 그 값을 갖는지는 나타내지 않는다. 벡터·회전·트랜스폼 같은 수학 구조체 값(예: `LocalSpawnOffset`)은 적지 않는다. 피해 행 값은 [이펙트 목록](../references/effect-list.md)에 있다.

| 몽타주 | 쓰는 곳 | 섹션 | 노티파이 |
|---|---|---|---|
| AM_HGTest_Attack_L_Air | GA_HGTest_Attack_Air | Default, Loop, Grounded | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_HGTest_Attack_Air} |
| AM_HGTest_Attack_DodgeCounter | GA_HGTest_Attack_DodgeCounter | 1, 2 | WxAnimNotifyState_SnapToTarget×2{TargetingPreset=TP_Attack_500; bSnapLocation=true}; WxAnimNotifyState_WeaponAttack×2{DamageDataRow=DT_Damage.AM_HGTest_Attack_DodgeCounter, DT_Damage.AM_Attack_LLLL}; WxAnimNotifyState_ComboWindow×2; WxAnimNotify_StartRecovery×2; WxAnimNotify_SpawnMinion{MinionClass=BP_Minion} |
| AM_HGTest_Attack_Heavy_1 | GA_HGTest_Attack_Heavy_1, GA_Minion_Attack_Heavy | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_500; bSnapLocation=true}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_HGTest_Attack_Heavy}; WxAnimNotify_StartRecovery |
| AM_HGTest_Attack_Heavy_2 | GA_HGTest_Attack_Heavy_2 | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_500; bSnapLocation=true}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_HGTest_Attack_Heavy}; WxAnimNotify_StartRecovery |
| AM_HGTest_Attack_Light_1 | GA_HGTest_Attack_Light_1 | 1, 2, 3, 4 | WxAnimNotifyState_SnapToTarget×4{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack×4{DamageDataRow=DT_Damage.AM_HGTest_Attack_Light_1}; WxAnimNotifyState_ComboWindow×4; WxAnimNotify_StartRecovery×4; WxAnimNotify_SpawnMinion{MinionClass=BP_Minion} |
| AM_HGTest_Attack_Light_2 | GA_HGTest_Attack_Light_2 | 1, 2, 3, 4 | WxAnimNotifyState_SnapToTarget×4{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack×4{DamageDataRow=DT_Damage.AM_HGTest_Attack_Light_2}; WxAnimNotifyState_ComboWindow×4; WxAnimNotify_StartRecovery×4 |
| AM_HGTest_Skill_1 | GA_HGTest_Skill_1 | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_500}; WxAnimNotify_SpawnMinion{MinionClass=BP_Minion}; WxAnimNotify_StartRecovery |
| AM_HGTest_Skill_2 | GA_HGTest_Skill_2 | Default | WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_HGTest_Skill_2}; WxAnimNotifyState_Rush{TargetSource=Minion; IgnoreCollisions=ObjectTypeQuery6, ObjectTypeQuery3, ObjectTypeQuery4, ObjectTypeQuery5, ObjectTypeQuery2, ObjectTypeQuery7}; WxAnimNotify_StartRecovery |
| AM_HGTest_Skill_3 | GA_HGTest_Skill_3 | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_500; bSnapLocation=true}; WxAnimNotifyState_WeaponAttack×3{DamageDataRow=DT_Damage.AM_HGTest_Skill_3}; WxAnimNotify_AreaDamage{TargetingPreset=TP_Attack_100}; WxAnimNotify_StartRecovery |
| AM_HGTest_Ultimate_1 | GA_HGTest_Ultimate_1 | Default | WxAnimNotify_SpawnMinion{MinionClass=BP_Doppelganger}; WxAnimNotify_SkillCutscene{Sequence=LS_HGTest_Ultimate_1}; WxAnimNotify_AreaDamage×3{TargetingPreset=TP_Attack_250; DamageDataRow=DT_Damage.AM_HGTest_Ultimate_1}; WxAnimNotify_StartRecovery |
| AM_HGTest_Ultimate_2 | GA_HGTest_Ultimate_2 | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_250}; WxAnimNotify_SkillCutscene{Sequence=LS_HGTest_Ultimate_2}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_HGTest_Ultimate_2}; WxAnimNotify_DespawnMinion{MinionClass=BP_Doppelganger}; WxAnimNotify_StartRecovery |
| AM_Minion_Skill_1 | GA_Minion_Skill_1 | Default | WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_Minion_Skill_1}; WxAnimNotify_StartRecovery |
| AM_Minion_Skill_2 | GA_Minion_Skill_2 | Default | WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_Minion_Skill_2}; WxAnimNotifyState_Rush{TargetSource=Master; IgnoreCollisions=ObjectTypeQuery6, ObjectTypeQuery3, ObjectTypeQuery4, ObjectTypeQuery5, ObjectTypeQuery2, ObjectTypeQuery7}; WxAnimNotify_StartRecovery |
| AM_Soldier_Pattern_1 | GA_Soldier_Pattern_1 | 1, 2, 3, 4 | WxAnimNotifyState_SnapToTarget×4{TargetingPreset=TP_Attack_250, TP_Attack_1000}; AnimNotify_GameplayCueState{GameplayCue=GameplayCue.AttackTelegraph.Red}; WxAnimNotifyState_ComboWindow; WxAnimNotifyState_WeaponAttack×3{DamageDataRow=DT_Damage.AM_Template_Pattern_1_1, DT_Damage.AM_Template_Pattern_1_2, DT_Damage.AM_Template_Pattern_1_3} |
| AM_Soldier_Pattern_2 | GA_Soldier_Pattern_2 | 1, 2, 3 | WxAnimNotifyState_SnapToTarget×3{TargetingPreset=TP_Attack_250, TP_Attack_1000; bSnapLocation=true}; AnimNotify_GameplayCueState{GameplayCue=GameplayCue.AttackTelegraph.Red}; WxAnimNotifyState_WeaponAttack×2{DamageDataRow=DT_Damage.AM_Template_Pattern_1_1, DT_Damage.AM_Template_Pattern_1_2} |
| AM_Soldier_Pattern_3 | GA_Soldier_Pattern_3 | 1, 2 | WxAnimNotifyState_SnapToTarget×2{TargetingPreset=TP_Attack_250, TP_Attack_1000}; AnimNotify_GameplayCueState{GameplayCue=GameplayCue.AttackTelegraph.Red}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_Template_Pattern_1_1} |
| AM_Template_Pattern_1 | GA_Template_Pattern_1 | 1, 2, 3, 4 | AnimNotify_GameplayCueState×4{GameplayCue=GameplayCue.AttackTelegraph.Red}; WxAnimNotifyState_SnapToTarget×4{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack×4{DamageDataRow=DT_Damage.AM_Template_Pattern_1_1, DT_Damage.AM_Template_Pattern_1_2, DT_Damage.AM_Template_Pattern_1_3, DT_Damage.AM_Template_Pattern_1_4} |
| AM_Template_Pattern_2 | GA_Template_Pattern_2 | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_10000}; AnimNotify_GameplayCueState{GameplayCue=GameplayCue.AttackTelegraph.Red}; WxAnimNotify_SpawnProjectile{ProjectileClass=BP_Template_Projectile} |
| AM_Template_Attack_L_Air | GA_Template_Attack_Air | Default, Loop, Grounded | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_Template_Attack_Air} |
| AM_Template_Attack_DodgeCounter | GA_Template_Attack_DodgeCounter | 1, 2 | WxAnimNotifyState_SnapToTarget×2{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack×2{DamageDataRow=DT_Damage.AM_Template_Attack_DodgeCounter_1, DT_Damage.AM_Template_Attack_DodgeCounter_2}; WxAnimNotifyState_ComboWindow×2; WxAnimNotify_StartRecovery×2 |
| AM_Template_Attack_H | GA_Template_Attack_Heavy | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_Template_Attack_H}; WxAnimNotify_StartRecovery |
| AM_Template_Attack_Light | GA_Template_Attack_Light | 1, 2, 3, 4 | WxAnimNotifyState_SnapToTarget×4{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack×4{DamageDataRow=DT_Damage.AM_Template_Attack_L, DT_Damage.AM_Template_Attack_LL, DT_Damage.AM_Template_Attack_LLL, DT_Damage.AM_Template_Attack_LLLL}; WxAnimNotifyState_ComboWindow×4; WxAnimNotify_StartRecovery×4 |
| AM_Template_Skill | GA_Template_Skill | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_250}; WxAnimNotifyState_WeaponAttack{DamageDataRow=DT_Damage.AM_Template_Skill}; WxAnimNotify_StartRecovery |
| AM_Template_Ultimate | GA_Template_Ultimate | Default | WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Attack_250}; WxAnimNotify_SkillCutscene{Sequence=LS_Template_Ultimate}; WxAnimNotify_AreaDamage{TargetingPreset=TP_Attack_250; DamageDataRow=DT_Damage.AM_Template_Ultimate} |
| AM_Shared_Dodge | GA_Shared_Dodge | Forward, ForwardRight, Right, BackRight, Back, BackLeft, Left, ForwardLeft, Backstep, SuccessForward, SuccessForwardRight, SuccessRight, SuccessBackRight, SuccessBack, SuccessBackLeft, SuccessLeft, SuccessForwardLeft | WxAnimNotifyState_ApplyGameplayEffect×17{EffectClass=WxEffect_Invincible}; WxAnimNotify_StartRecovery×16; AnimNotify_GameplayCue×8{GameplayCue=GameplayCue.GhostTrail}; WxAnimNotifyState_SlowTime×8 |
| AM_Shared_Finisher | GA_Shared_Finisher | Default | AnimNotifyState_MotionWarping{RootMotionModifier=RootMotionModifier_SkewWarp}; WxAnimNotify_FinisherVictim{VictimMontage=AM_Shared_HitReact_Finisher}; WxAnimNotifyState_CameraMove; WxAnimNotify_FinisherDamage{DamageDataRow=DT_Damage.AM_Shared_Finisher} |
| AM_Shared_HitReact_Finisher | AM_Shared_Finisher | Default |  |
| AM_Shared_Groggy | GA_Shared_Groggy | Default |  |
| AM_Shared_Guard | GA_Shared_Guard | Default, LoopStart | WxAnimNotifyState_ApplyGameplayEffect{EffectClass=WxEffect_PerfectGuard}; WxAnimNotifyState_SnapToTarget{TargetingPreset=TP_Guard} |
| AM_Shared_GuardReact | GA_Shared_GuardReact | GuardHit, GuardKnockback, GuardBreak, PerfectGuard | WxAnimNotifyState_ApplyGameplayEffect{EffectClass=WxEffect_PerfectGuard}; WxAnimNotifyState_SlowTime |
| AM_Shared_HitReact | GA_Shared_HitReact_Normal | Normal |  |
| AM_Shared_HitReact_Knock | GA_Shared_HitReact | KnockBack, KnockDown, Parry |  |
| AM_Shared_HitReact_Knockup | GA_Shared_HitReact_KnockUp | KnockUp, Loop, Grounded |  |
| AM_Shared_UseItem | GA_Shared_UseItem | Default | WxAnimNotify_UseItem; AnimNotify_PlayNiagaraEffect{Template=NS_SkeletalMeshTris_Burst}; WxAnimNotify_StartRecovery |

## 관련 문서

- [[character-list|캐릭터 목록]] ([캐릭터 목록](../references/character-list.md))
- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))
- [[effect-list|이펙트 목록]] ([이펙트 목록](../references/effect-list.md))

## Sources

- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — GA_가 가진 데이터와 배치 규칙
