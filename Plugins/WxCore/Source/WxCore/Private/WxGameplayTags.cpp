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

	UE_DEFINE_GAMEPLAY_TAG(Ability, "Ability");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");

	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Dodge, "Input.Dodge");

	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Game, "UI.Layer.Game");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_GameMenu, "UI.Layer.GameMenu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Menu, "UI.Layer.Menu");
	UE_DEFINE_GAMEPLAY_TAG(UI_Layer_Modal, "UI.Layer.Modal");
}
