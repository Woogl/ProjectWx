// Copyright Woogle. All Rights Reserved.

#include "WxGameplayTags.h"

namespace WxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Ragdoll, "State.Ragdoll");
	UE_DEFINE_GAMEPLAY_TAG(State_Groggy, "State.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(State_LockOn, "State.LockOn");
	UE_DEFINE_GAMEPLAY_TAG(State_LockedOn, "State.LockedOn");
	UE_DEFINE_GAMEPLAY_TAG(State_InCombat, "State.InCombat");
	UE_DEFINE_GAMEPLAY_TAG(State_Invincible, "State.Invincible");
	UE_DEFINE_GAMEPLAY_TAG(State_Guard, "State.Guard");
	UE_DEFINE_GAMEPLAY_TAG(State_PerfectGuard, "State.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(State_HitReact, "State.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(State_SuperArmor, "State.SuperArmor");
	UE_DEFINE_GAMEPLAY_TAG(State_Finisher, "State.Finisher");

	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact, "Event.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Normal, "Event.HitReact.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_KnockBack, "Event.HitReact.KnockBack");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_KnockDown, "Event.HitReact.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_KnockUp, "Event.HitReact.KnockUp");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Parry, "Event.HitReact.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Finisher, "Event.HitReact.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Backstab, "Event.HitReact.Backstab");
	UE_DEFINE_GAMEPLAY_TAG(Event_DodgeSuccess, "Event.DodgeSuccess");
	UE_DEFINE_GAMEPLAY_TAG(Event_PerfectGuard, "Event.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(Event_UseItem, "Event.UseItem");
	UE_DEFINE_GAMEPLAY_TAG(Event_Interact, "Event.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Event_Finisher, "Event.Finisher");
	UE_DEFINE_GAMEPLAY_TAG(Event_Backstab, "Event.Backstab");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitStop, "Event.HitStop");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Door_Close, "Gimmick.Door.Close");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Door_Open, "Gimmick.Door.Open");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Elevator_Closed, "Gimmick.Elevator.Closed");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Elevator_AtStart, "Gimmick.Elevator.AtStart");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Elevator_AtEnd, "Gimmick.Elevator.AtEnd");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_SpawnConsole_Idle, "Gimmick.SpawnConsole.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_SpawnConsole_Spawned, "Gimmick.SpawnConsole.Spawned");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_AlarmConsole_Idle, "Gimmick.AlarmConsole.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_AlarmConsole_Alarmed, "Gimmick.AlarmConsole.Alarmed");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_CutsceneTrigger_Idle, "Gimmick.CutsceneTrigger.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_CutsceneTrigger_Playing, "Gimmick.CutsceneTrigger.Playing");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_TreasureChest_Closed, "Gimmick.TreasureChest.Closed");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_TreasureChest_Open, "Gimmick.TreasureChest.Open");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_CheckPoint_Unlit, "Gimmick.CheckPoint.Unlit");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_CheckPoint_Lit, "Gimmick.CheckPoint.Lit");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_LaserCorridor_Active, "Gimmick.LaserCorridor.Active");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_LaserCorridor_Deactivated, "Gimmick.LaserCorridor.Deactivated");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Restore, "Gimmick.Restore");

	UE_DEFINE_GAMEPLAY_TAG(ANS_WeaponCollision, "ANS.WeaponCollision");
	UE_DEFINE_GAMEPLAY_TAG(ANS_ComboWindow, "ANS.ComboWindow");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage, "GameplayCue.Damage");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_PerfectGuard, "GameplayCue.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Exceed, "GameplayCue.Exceed");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Burn, "GameplayCue.Burn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Red, "GameplayCue.AttackTelegraph.Red");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Yellow, "GameplayCue.AttackTelegraph.Yellow");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Blue, "GameplayCue.AttackTelegraph.Blue");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_AttackTelegraph_Purple, "GameplayCue.AttackTelegraph.Purple");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Critical, "Damage.Critical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Unblockable, "Damage.Unblockable");
	UE_DEFINE_GAMEPLAY_TAG(Damage_ParryHitReact, "Damage.ParryHitReact");

	UE_DEFINE_GAMEPLAY_TAG(Ability, "Ability");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Guard, "Ability.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill, "Ability.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_1, "Ability.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_2, "Ability.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_3, "Ability.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_4, "Ability.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ultimate, "Ability.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Interact, "Ability.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_UseItem, "Ability.UseItem");

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
	UE_DEFINE_GAMEPLAY_TAG(Ability_Pattern_10, "Ability.Pattern.10");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Duration, "SetByCaller.Duration");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Recovery_UP, "SetByCaller.Recovery.UP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Recovery_MP, "SetByCaller.Recovery.MP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_ReflectDP, "SetByCaller.ReflectDP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Coeff_ATK, "SetByCaller.Coeff.ATK");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_RawDamage, "SetByCaller.RawDamage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_HitStop, "SetByCaller.HitStop");

	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack_Light, "Input.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack_Heavy, "Input.Attack.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Input_Dodge, "Input.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Input_Sprint, "Input.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Input_Guard, "Input.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_1, "Input.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_2, "Input.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_3, "Input.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_4, "Input.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ultimate, "Input.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Input_LockOn, "Input.LockOn");
	UE_DEFINE_GAMEPLAY_TAG(Input_UseItem, "Input.UseItem");
	
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game, "UI.Layer.Game");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu, "UI.Layer.GameMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu, "UI.Layer.Menu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal, "UI.Layer.Modal");

	UE_DEFINE_GAMEPLAY_TAG(UI_Action_Inventory, "UI.Action.Inventory");
	UE_DEFINE_GAMEPLAY_TAG(UI_Action_MainMenu, "UI.Action.MainMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Action_FreeCursor, "UI.Action.FreeCursor");
}
