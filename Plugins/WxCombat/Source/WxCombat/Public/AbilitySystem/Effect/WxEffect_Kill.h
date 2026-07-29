// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Kill.generated.h"

/**
 * 즉사 GameplayEffect.
 *
 * Instant 정책으로, Target의 현재 HP를 그대로 IncomingDamage로 넣어 방어력과 무관하게 확정 처치한다.
 * ExecCalc 없이 HP를 캡처하는 AttributeBased 모디파이어만으로 계산한다.
 * IncomingDamage 경로를 타므로 PostGameplayEffectExecute의 사망 처리(State_Dead 부여·사망 어빌리티·AI 보고)가 정상 발동한다.
 * 연출된 즉사이므로 대미지 수치 플로터(GameplayCue_Damage)는 발행하지 않는다.
 *
 * 대상을 반드시 처치해야 하는 경로에서 사용한다 — 현재 사용처는 개발용 치트(UWxCheatManager::WxKillPlayer) 하나뿐이다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Kill : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Kill();
};
