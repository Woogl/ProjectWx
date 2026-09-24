// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_SkillCutscene.generated.h"

/**
 * 스킬 컷신 동안 입력형 어빌리티 발동을 막는다. 피격·사망 같은 반응은 막지 않는다.
 * 수명은 UWxSkillCutsceneComponent의 세션이 쥔다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_SkillCutscene : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_SkillCutscene();
};
