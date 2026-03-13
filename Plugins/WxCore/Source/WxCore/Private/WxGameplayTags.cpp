// Copyright Woogle. All Rights Reserved.

#include "WxGameplayTags.h"

namespace WxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Airborne, "State.Airborne");

	UE_DEFINE_GAMEPLAY_TAG(Event_HitReact, "Event.HitReact");

	UE_DEFINE_GAMEPLAY_TAG(ANS_WeaponCollision, "ANS.WeaponCollision");
	UE_DEFINE_GAMEPLAY_TAG(ANS_ComboWindow, "ANS.ComboWindow");
	UE_DEFINE_GAMEPLAY_TAG(ANS_Invincible, "ANS.Invincible");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Combo1, "Ability.Attack.Combo1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Combo2, "Ability.Attack.Combo2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Combo3, "Ability.Attack.Combo3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Combo4, "Ability.Attack.Combo4");

	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Dodge, "Input.Dodge");
}
