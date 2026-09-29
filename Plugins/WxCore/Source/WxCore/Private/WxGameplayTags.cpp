// Copyright Woogle. All Rights Reserved.

#include "WxGameplayTags.h"

namespace WxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Character_Boss, "Character.Boss");

	UE_DEFINE_GAMEPLAY_TAG(State_Engaged, "State.Engaged");
	UE_DEFINE_GAMEPLAY_TAG(State_Dialogue, "State.Dialogue");
	UE_DEFINE_GAMEPLAY_TAG(State_Ragdoll, "State.Ragdoll");

	UE_DEFINE_GAMEPLAY_TAG(Master_Minion, "Master.Minion");
	UE_DEFINE_GAMEPLAY_TAG(Master_Doppelganger, "Master.Doppelganger");

	UE_DEFINE_GAMEPLAY_TAG(Effect_Invincible, "Effect.Invincible");
	UE_DEFINE_GAMEPLAY_TAG(Effect_GuardReduction, "Effect.GuardReduction");
	UE_DEFINE_GAMEPLAY_TAG(Effect_PerfectGuard, "Effect.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Exhausted, "Effect.Exhausted");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SuperArmor, "Effect.SuperArmor");
	UE_DEFINE_GAMEPLAY_TAG(Effect_HitStop, "Effect.HitStop");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SkillCutscene, "Effect.SkillCutscene");
	UE_DEFINE_GAMEPLAY_TAG(Effect_IgnoreAggro, "Effect.IgnoreAggro");
	UE_DEFINE_GAMEPLAY_TAG(Effect_IgnoreCosts, "Effect.IgnoreCosts");
	UE_DEFINE_GAMEPLAY_TAG(Effect_IgnoreAbilityActivationTags, "Effect.IgnoreAbilityActivationTags");

	UE_DEFINE_GAMEPLAY_TAG(Movement_InAir, "Movement.InAir");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Sprint, "Movement.Sprint");

	UE_DEFINE_GAMEPLAY_TAG(HitReact, "HitReact");
	UE_DEFINE_GAMEPLAY_TAG(HitReact_Normal, "HitReact.Normal");
	UE_DEFINE_GAMEPLAY_TAG(HitReact_KnockBack, "HitReact.KnockBack");
	UE_DEFINE_GAMEPLAY_TAG(HitReact_KnockDown, "HitReact.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(HitReact_KnockUp, "HitReact.KnockUp");

	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_ActionPhaseChanged, "Event.Ability.ActionPhaseChanged");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit, "Event.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Parry, "Event.Hit.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_GuardBreak, "Event.Hit.GuardBreak");
	UE_DEFINE_GAMEPLAY_TAG(Event_DamageDealt, "Event.DamageDealt");
	UE_DEFINE_GAMEPLAY_TAG(Event_PerfectGuard, "Event.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(Event_Interact, "Event.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Event_Finisher, "Event.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");
	UE_DEFINE_GAMEPLAY_TAG(Event_Groggy, "Event.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Event_UseItem, "Event.UseItem");
	UE_DEFINE_GAMEPLAY_TAG(Event_ApplyFinisherDamage, "Event.ApplyFinisherDamage");
	UE_DEFINE_GAMEPLAY_TAG(Event_PlayFinisherVictimMontage, "Event.PlayFinisherVictimMontage");
	
	UE_DEFINE_GAMEPLAY_TAG(Device_Button_Idle, "Device.Button.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Device_Button_Pressed, "Device.Button.Pressed");

	UE_DEFINE_GAMEPLAY_TAG(Device_Door_Close, "Device.Door.Close");
	UE_DEFINE_GAMEPLAY_TAG(Device_Door_Open, "Device.Door.Open");

	UE_DEFINE_GAMEPLAY_TAG(Device_Elevator_Inactive, "Device.Elevator.Inactive");
	UE_DEFINE_GAMEPLAY_TAG(Device_Elevator_Idle, "Device.Elevator.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Device_Elevator_Moving, "Device.Elevator.Moving");

	UE_DEFINE_GAMEPLAY_TAG(Device_TreasureChest_Closed, "Device.TreasureChest.Closed");
	UE_DEFINE_GAMEPLAY_TAG(Device_TreasureChest_Open, "Device.TreasureChest.Open");

	UE_DEFINE_GAMEPLAY_TAG(Device_CheckPoint_Unlit, "Device.CheckPoint.Unlit");
	UE_DEFINE_GAMEPLAY_TAG(Device_CheckPoint_Lit, "Device.CheckPoint.Lit");
	UE_DEFINE_GAMEPLAY_TAG(Device_CheckPoint_Resting, "Device.CheckPoint.Resting");

	UE_DEFINE_GAMEPLAY_TAG(Device_Piston_On, "Device.Piston.On");
	UE_DEFINE_GAMEPLAY_TAG(Device_Piston_Off, "Device.Piston.Off");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_DamageFloater, "GameplayCue.DamageFloater");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hit, "GameplayCue.Hit");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_PerfectGuard, "GameplayCue.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_GhostTrail, "GameplayCue.GhostTrail");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Exceed, "GameplayCue.Exceed");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Doppelganger, "GameplayCue.Doppelganger");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Red, "GameplayCue.AttackTelegraph.Red");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Yellow, "GameplayCue.AttackTelegraph.Yellow");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Blue, "GameplayCue.AttackTelegraph.Blue");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Purple, "GameplayCue.AttackTelegraph.Purple");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Critical, "Damage.Critical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_GuardBreak, "Damage.GuardBreak");
	UE_DEFINE_GAMEPLAY_TAG(Damage_PerfectGuarded, "Damage.PerfectGuarded");
	UE_DEFINE_GAMEPLAY_TAG(Damage_CanCritical, "Damage.CanCritical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_CanGuard, "Damage.CanGuard");
	UE_DEFINE_GAMEPLAY_TAG(Damage_CanParry, "Damage.CanParry");

	UE_DEFINE_GAMEPLAY_TAG(Ability, "Ability");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Action, "Ability.Action");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Attack, "Ability.Action.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Attack_Light, "Ability.Action.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Attack_Heavy, "Ability.Action.Attack.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Attack_Air, "Ability.Action.Attack.Air");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Attack_DodgeCounter, "Ability.Action.Attack.DodgeCounter");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill, "Ability.Action.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill_1, "Ability.Action.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill_2, "Ability.Action.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill_3, "Ability.Action.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill_4, "Ability.Action.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Ultimate, "Ability.Action.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Dodge, "Ability.Action.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Guard, "Ability.Action.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_UseItem, "Ability.Action.UseItem");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Interact, "Ability.Action.Interact");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern, "Ability.Action.Pattern");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_1, "Ability.Action.Pattern.1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_2, "Ability.Action.Pattern.2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_3, "Ability.Action.Pattern.3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_4, "Ability.Action.Pattern.4");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_5, "Ability.Action.Pattern.5");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_6, "Ability.Action.Pattern.6");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_7, "Ability.Action.Pattern.7");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_8, "Ability.Action.Pattern.8");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Pattern_9, "Ability.Action.Pattern.9");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_LockOn, "Ability.LockOn");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Passive, "Ability.Passive");

	UE_DEFINE_GAMEPLAY_TAG(Ability_HitReact, "Ability.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_GuardReact, "Ability.GuardReact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Groggy, "Ability.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Death, "Ability.Death");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Finisher, "Ability.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Ability_PlayMontageOnce, "Ability.PlayMontageOnce");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown, "Cooldown");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dodge, "Cooldown.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_1, "Cooldown.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_2, "Cooldown.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_3, "Cooldown.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Ultimate, "Cooldown.Ultimate");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Magnitude, "SetByCaller.Magnitude");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Duration, "SetByCaller.Duration");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Coeff_ATK, "SetByCaller.Coeff.ATK");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MoveSpeedScale, "SetByCaller.MoveSpeedScale");
	
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game, "UI.Layer.Game");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu, "UI.Layer.GameMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu, "UI.Layer.Menu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal, "UI.Layer.Modal");

	UE_DEFINE_GAMEPLAY_TAG(UI_Action_Inventory, "UI.Action.Inventory");
	UE_DEFINE_GAMEPLAY_TAG(UI_Action_MainMenu, "UI.Action.MainMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Action_FreeCursor, "UI.Action.FreeCursor");
}
