// Copyright Woogle. All Rights Reserved.

#include "WxGameplayTags.h"

namespace WxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Aerial, "State.Aerial");
	UE_DEFINE_GAMEPLAY_TAG(State_Groggy, "State.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(State_LockOn, "State.LockOn");
	UE_DEFINE_GAMEPLAY_TAG(State_Invincible, "State.Invincible");

	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact, "Event.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Normal, "Event.HitReact.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Knockback, "Event.HitReact.Knockback");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Knockdown, "Event.HitReact.Knockdown");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact_Knockup, "Event.HitReact.Knockup");
	UE_DEFINE_GAMEPLAY_TAG(Event_GuardHit, "Event.GuardHit");
	UE_DEFINE_GAMEPLAY_TAG(Event_GuardHit_Normal, "Event.GuardHit.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Event_GuardHit_Knockback, "Event.GuardHit.Knockback");
	UE_DEFINE_GAMEPLAY_TAG(Event_DodgeSuccess, "Event.DodgeSuccess");
	UE_DEFINE_GAMEPLAY_TAG(Event_PerfectGuard, "Event.PerfectGuard");
	UE_DEFINE_GAMEPLAY_TAG(Event_AttackHit, "Event.AttackHit");

	UE_DEFINE_GAMEPLAY_TAG(ANS_WeaponCollision, "ANS.WeaponCollision");
	UE_DEFINE_GAMEPLAY_TAG(ANS_ComboWindow, "ANS.ComboWindow");
	UE_DEFINE_GAMEPLAY_TAG(State_Guard, "State.Guard");
	UE_DEFINE_GAMEPLAY_TAG(ANS_PerfectGuard, "ANS.PerfectGuard");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage, "GameplayCue.Damage");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_BuffATK, "GameplayCue.BuffATK");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Burn, "GameplayCue.Burn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitStop, "GameplayCue.HitStop");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Critical, "Damage.Critical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_SuppressFloater, "Damage.SuppressFloater");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Unblockable, "Damage.Unblockable");

	UE_DEFINE_GAMEPLAY_TAG(Ability, "Ability");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Jump, "Ability.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Guard, "Ability.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_1, "Ability.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_2, "Ability.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_3, "Ability.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill_4, "Ability.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ultimate, "Ability.Ultimate");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Jump, "Cooldown.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dodge, "Cooldown.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Guard, "Cooldown.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_1, "Cooldown.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_2, "Cooldown.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_3, "Cooldown.Skill.3");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill_4, "Cooldown.Skill.4");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Ultimate, "Cooldown.Ultimate");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Cost, "SetByCaller.Cost");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Recovery, "SetByCaller.Recovery");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_ReflectDP, "SetByCaller.ReflectDP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Coeff_ATK, "SetByCaller.Coeff.ATK");

	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump");
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

	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game, "UI.Layer.Game");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu, "UI.Layer.GameMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu, "UI.Layer.Menu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal, "UI.Layer.Modal");
}
