// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

/**
 * 프로젝트 커스텀 콜리전 채널 정의.
 * DefaultEngine.ini의 채널 등록 순서와 일치해야 한다.
 *
 * ECC_WxAttack: 무기·투사체 히트박스의 Object Type으로 사용하는 Object Channel.
 *               DefaultResponse=Block 이므로 별도 override 없는 프로파일(WorldStatic/WorldDynamic/BlockAll 등)은 WxAttack을 Block한다.
 *               캐릭터 메시는 WxAttack에 Overlap으로, 캡슐은 Ignore로 명시 override하여 메시에서만 피격 판정이 일어난다.
 *               투사체는 "WxProjectile" 프리셋을 사용한다.
 *
 * ECC_WxInteractable: 상호작용 볼륨 표식용 Channel. UWxInteractionComponent 가 지정한 볼륨 메시의 응답을 Overlap 으로 켠다.
 *                     DefaultResponse=Ignore 이므로 응답을 켜지 않은 다른 메시엔 무영향이고, 다른 쿼리/트레이스에도 끼어들지 않는다.
 *                     플레이어 측 스캐너가 OverlapMultiByChannel(ECC_WxInteractable) 으로 주변 상호작용 볼륨을 수집한다.
 */
inline constexpr ECollisionChannel ECC_WxAttack = ECC_GameTraceChannel1;
inline constexpr ECollisionChannel ECC_WxInteractable = ECC_GameTraceChannel2;
