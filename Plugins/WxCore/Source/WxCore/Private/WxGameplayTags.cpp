// Copyright Woogle. All Rights Reserved.

#include "WxGameplayTags.h"

namespace WxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_LockedOn, "State.LockedOn");
	UE_DEFINE_GAMEPLAY_TAG(State_InCombat, "State.InCombat");
	UE_DEFINE_GAMEPLAY_TAG(State_BeingFinished, "State.BeingFinished");
	UE_DEFINE_GAMEPLAY_TAG(State_Dialogue, "State.Dialogue");

	UE_DEFINE_GAMEPLAY_TAG(Effect_Invincible, "Effect.Invincible");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Guard, "Effect.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Effect_PerfectGuard, "Effect.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Exhausted, "Effect.Exhausted");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SuperArmor, "Effect.SuperArmor");

	UE_DEFINE_GAMEPLAY_TAG(Movement_InAir, "Movement.InAir");
	UE_DEFINE_GAMEPLAY_TAG(Movement_Sprint, "Movement.Sprint");

	UE_DEFINE_GAMEPLAY_TAG(Event_Hit, "Event.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Normal, "Event.Hit.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_KnockBack, "Event.Hit.KnockBack");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_KnockDown, "Event.Hit.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_KnockUp, "Event.Hit.KnockUp");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Parry, "Event.Hit.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Finisher, "Event.Hit.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Backstab, "Event.Hit.Backstab");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_GuardBreak, "Event.Hit.GuardBreak");
	UE_DEFINE_GAMEPLAY_TAG(Event_DamageDealt, "Event.DamageDealt");
	UE_DEFINE_GAMEPLAY_TAG(Event_DodgeSuccess, "Event.DodgeSuccess");
	UE_DEFINE_GAMEPLAY_TAG(Event_PerfectGuard, "Event.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(Event_UseItem, "Event.UseItem");
	UE_DEFINE_GAMEPLAY_TAG(Event_Interact, "Event.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Event_Device_Triggered, "Event.Device.Triggered");
	UE_DEFINE_GAMEPLAY_TAG(Event_Finisher, "Event.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Event_Backstab, "Event.Backstab");
	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");
	UE_DEFINE_GAMEPLAY_TAG(Event_Groggy, "Event.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ragdoll, "Event.Ragdoll");
	
	UE_DEFINE_GAMEPLAY_TAG(Device_Button_Idle, "Device.Button.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Device_Button_Pressed, "Device.Button.Pressed");
	UE_DEFINE_GAMEPLAY_TAG(Device_Button_Locked, "Device.Button.Locked");

	UE_DEFINE_GAMEPLAY_TAG(Device_Door_Close, "Device.Door.Close");
	UE_DEFINE_GAMEPLAY_TAG(Device_Door_Open, "Device.Door.Open");

	UE_DEFINE_GAMEPLAY_TAG(Device_Elevator_Inactive, "Device.Elevator.Inactive");
	UE_DEFINE_GAMEPLAY_TAG(Device_Elevator_1F, "Device.Elevator.1F");
	UE_DEFINE_GAMEPLAY_TAG(Device_Elevator_2F, "Device.Elevator.2F");

	UE_DEFINE_GAMEPLAY_TAG(Device_TreasureChest_Closed, "Device.TreasureChest.Closed");
	UE_DEFINE_GAMEPLAY_TAG(Device_TreasureChest_Open, "Device.TreasureChest.Open");

	UE_DEFINE_GAMEPLAY_TAG(Device_CheckPoint_Unlit, "Device.CheckPoint.Unlit");
	UE_DEFINE_GAMEPLAY_TAG(Device_CheckPoint_Lit, "Device.CheckPoint.Lit");

	UE_DEFINE_GAMEPLAY_TAG(Device_Piston_On, "Device.Piston.On");
	UE_DEFINE_GAMEPLAY_TAG(Device_Piston_Off, "Device.Piston.Off");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_DamageFloater, "GameplayCue.DamageFloater");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hit, "GameplayCue.Hit");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_PerfectGuard, "GameplayCue.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_GhostTrail, "GameplayCue.GhostTrail");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Exceed, "GameplayCue.Exceed");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Red, "GameplayCue.AttackTelegraph.Red");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Yellow, "GameplayCue.AttackTelegraph.Yellow");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Blue, "GameplayCue.AttackTelegraph.Blue");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Purple, "GameplayCue.AttackTelegraph.Purple");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Critical, "Damage.Critical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_CanCritical, "Damage.CanCritical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_CanGuard, "Damage.CanGuard");
	UE_DEFINE_GAMEPLAY_TAG(Damage_CanParry, "Damage.CanParry");

	UE_DEFINE_GAMEPLAY_TAG(Ability, "Ability");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Light, "Ability.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Heavy, "Ability.Attack.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Air, "Ability.Attack.Air");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_DodgeCounter, "Ability.Attack.DodgeCounter");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Guard, "Ability.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_GuardReact, "Ability.GuardReact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill, "Ability.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_1, "Ability.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_2, "Ability.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_3, "Ability.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_4, "Ability.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ultimate, "Ability.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Interact, "Ability.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_UseItem, "Ability.UseItem");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Finisher, "Ability.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Ability_LockOn, "Ability.LockOn");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Passive, "Ability.Passive");

	UE_DEFINE_GAMEPLAY_TAG(Ability_HitReact, "Ability.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Groggy, "Ability.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Death, "Ability.Death");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern, "Ability.Pattern");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_1, "Ability.Pattern.1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_2, "Ability.Pattern.2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_3, "Ability.Pattern.3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_4, "Ability.Pattern.4");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_5, "Ability.Pattern.5");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_6, "Ability.Pattern.6");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_7, "Ability.Pattern.7");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_8, "Ability.Pattern.8");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_9, "Ability.Pattern.9");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Duration, "SetByCaller.Duration");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_DP, "SetByCaller.DP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Coeff_ATK, "SetByCaller.Coeff.ATK");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_RawDamage, "SetByCaller.RawDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MoveSpeedScale, "SetByCaller.MoveSpeedScale");
	
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game, "UI.Layer.Game");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu, "UI.Layer.GameMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu, "UI.Layer.Menu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal, "UI.Layer.Modal");

	UE_DEFINE_GAMEPLAY_TAG(UI_Action_Inventory, "UI.Action.Inventory");
	UE_DEFINE_GAMEPLAY_TAG(UI_Action_MainMenu, "UI.Action.MainMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Action_FreeCursor, "UI.Action.FreeCursor");
}
