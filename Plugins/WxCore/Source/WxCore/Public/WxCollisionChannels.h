// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/**
 * 프로젝트 커스텀 콜리전 채널 정의.
 * DefaultEngine.ini의 채널 등록 순서와 일치해야 한다.
 *
 * Attack: 무기·투사체의 Object Type으로 사용하는 Object Channel.
 *         캐릭터 메쉬가 Overlap으로 응답하여 피격 판정 대상이 되며, WorldStatic과 WorldDynamic은 Block으로 응답
 */
namespace WxCollision
{
	inline constexpr ECollisionChannel Attack = ECC_GameTraceChannel1;
}
