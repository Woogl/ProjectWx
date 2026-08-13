// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "WxAbilitySystemGlobals.generated.h"

/**
 * MakeEffectContext가 FWxCombatEffectContext를 만들게 해, 대미지 진입점마다 컨텍스트를 따로 챙기지 않아도 되게 한다.
 * DefaultGame.ini의 AbilitySystemGlobalsClassName으로 등록하며, 등록이 빠지면 대미지 판정 결과가 실리지 못해 UWxExecCalc_Damage가 ensure로 알린다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;

	virtual void InitGameplayCueParameters(FGameplayCueParameters& CueParameters, const FGameplayEffectContextHandle& EffectContext) override;
};
