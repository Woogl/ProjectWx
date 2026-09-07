// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "WxAbilitySystemGlobals.generated.h"

/**
 * GE가 들고 다니는 큐의 발생 위치를 컨텍스트의 히트 결과에서 채운다.
 * DefaultGame.ini의 AbilitySystemGlobalsClassName으로 등록하며, 등록이 빠지면 그 큐들이 원점에서 터진다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual void InitGameplayCueParameters(FGameplayCueParameters& CueParameters, const FGameplayEffectContextHandle& EffectContext) override;
};
