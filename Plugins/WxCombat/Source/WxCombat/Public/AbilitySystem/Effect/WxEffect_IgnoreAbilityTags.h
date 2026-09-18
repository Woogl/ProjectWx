// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "NativeGameplayTags.h"
#include "WxEffect_IgnoreAbilityTags.generated.h"

namespace WxCombatGameplayTags
{
	WXCOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_IgnoreAbilityTags);
}

/** 유지되는 동안 UWxAbilityBase의 발동 태그 조건을 면제한다. 코스트·쿨다운·동작 점유는 별도로 검사한다. */
UCLASS()
class WXCOMBAT_API UWxEffect_IgnoreAbilityTags : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_IgnoreAbilityTags();
};
