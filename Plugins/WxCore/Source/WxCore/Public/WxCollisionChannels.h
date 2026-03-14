// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/**
 * 프로젝트 커스텀 콜리전 채널 정의.
 * DefaultEngine.ini의 채널 등록 순서와 일치해야 한다.
 *
 * Attack: 무기·투사체의 Object Type으로 사용하는 Object Channel.
 *         캐릭터 Capsule이 Overlap으로 응답하여 피격 판정 대상이 됨.
 *         Targeting System의 AOE Task에서 CollisionObjectTypes로 지정하여 타겟 탐색에도 활용.
 */
namespace WxCollision
{
	inline constexpr ECollisionChannel Attack = ECC_GameTraceChannel1;
}
