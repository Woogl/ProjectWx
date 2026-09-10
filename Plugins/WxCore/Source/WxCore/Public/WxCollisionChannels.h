// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/**
 * DefaultEngine.ini의 WxAttack 등록 항목이 쓰는 Channel 값과 일치해야 한다. 항목은 Channel을 키로 명시 바인딩하므로 줄 순서는 무관하고, 그 값을 바꾸거나 항목을 지우면 아래 판정이 전부 다른 채널을 가리킨다.
 *
 * ECC_WxAttack: 무기·투사체 히트박스의 Object Type으로 사용하는 Object Channel.
 *               DefaultResponse=Block 이므로 별도 override 없는 프로파일(WorldStatic/WorldDynamic/BlockAll 등)은 WxAttack을 Block한다.
 *               캐릭터 메시는 WxAttack에 Overlap으로, 캡슐은 Ignore로 명시 override하여 메시에서만 피격 판정이 일어난다.
 *               투사체는 "WxProjectile" 프리셋을 사용한다.
 */
inline constexpr ECollisionChannel ECC_WxAttack = ECC_GameTraceChannel1;
