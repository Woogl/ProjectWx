// Copyright Woogle. All Rights Reserved.

#include "WxGameplayTags.h"

namespace WxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Aerial, "State.Aerial");
	UE_DEFINE_GAMEPLAY_TAG(State_Groggy, "State.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(State_LockOn, "State.LockOn");

	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact, "Event.HitReact");

	UE_DEFINE_GAMEPLAY_TAG(ANS_WeaponCollision, "ANS.WeaponCollision");
	UE_DEFINE_GAMEPLAY_TAG(ANS_ComboWindow, "ANS.ComboWindow");
	UE_DEFINE_GAMEPLAY_TAG(ANS_Invincible, "ANS.Invincible");
	UE_DEFINE_GAMEPLAY_TAG(ANS_Guard, "ANS.Guard");
	UE_DEFINE_GAMEPLAY_TAG(ANS_PerfectGuard, "ANS.PerfectGuard");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage, "GameplayCue.Damage");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_BuffATK, "GameplayCue.BuffATK");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Bleed, "GameplayCue.Bleed");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Critical, "Damage.Critical");

	UE_DEFINE_GAMEPLAY_TAG(Ability, "Ability");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Guard, "Ability.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Jump, "Ability.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Skill, "Ability.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ultimate, "Ability.Ultimate");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Attack, "Cooldown.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dodge, "Cooldown.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Guard, "Cooldown.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Jump, "Cooldown.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill, "Cooldown.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Ultimate, "Cooldown.Ultimate");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Cost_MP, "SetByCaller.Cost.MP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Cost_UP, "SetByCaller.Cost.UP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_ReflectDP, "SetByCaller.ReflectDP");

	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Dodge, "Input.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Input_Guard, "Input.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill, "Input.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Input_Sprint, "Input.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ultimate, "Input.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Input_LockOn, "Input.LockOn");

	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game, "UI.Layer.Game");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu, "UI.Layer.GameMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu, "UI.Layer.Menu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal, "UI.Layer.Modal");
}
