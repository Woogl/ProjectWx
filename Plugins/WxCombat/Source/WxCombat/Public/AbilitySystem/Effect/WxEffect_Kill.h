// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Kill.generated.h"

/**
 * Instant 정책으로, Target의 현재 HP를 그대로 IncomingDamage로 넣어 방어력과 무관하게 확정 처치한다.
 * IncomingDamage 경로를 타므로 PostGameplayEffectExecute의 사망 처리(Event.Death 송출·사망 어빌리티)가 정상 발동한다.
 * 대미지 수치 플로터(GameplayCue_DamageFloater)는 UWxCombatAttributeSet이 UWxEffect_Damage 스펙만 골라 발행하므로 이 GE로는 뜨지 않는다.
 *
 * 현재 사용처는 개발용 치트(UWxCheatManager의 WxKillPlayer·WxKillEnemies)뿐이다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Kill : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Kill();
};
