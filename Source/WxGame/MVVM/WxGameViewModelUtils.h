// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWxCharacterBase;
class UWxViewModel_Character;

namespace WxGameViewModel
{
	/** 캐릭터의 ASC·이름으로 VM 을 초기화하고(같은 ASC 면 기존 AbilitySystem VM 유지), 그 AbilitySystem VM 에 게임의 GE 표시 데이터를 연결한다. */
	void InitializeCharacter(UWxViewModel_Character& ViewModel, const AWxCharacterBase& Character);
}
