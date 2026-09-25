// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_IgnoreAbilityActivationTags.generated.h"

/** 소유자의 ActivationRequiredTags·ActivationBlockedTags만 면제한다. 어빌리티·GE의 차단은 유지한다. */
UCLASS()
class WXCOMBAT_API UWxEffect_IgnoreAbilityActivationTags : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_IgnoreAbilityActivationTags();
};
