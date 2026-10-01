// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_GuardReduction.generated.h"

/**
 * 방어 유효 상태(Effect.GuardReduction)를 부여하고 경감률을 GuardReductionScale에 싣는다.
 * 고정 지속시간 없이 가드 어빌리티 수명에 묶여 걷힌다.
 *
 * 경감률과 표시 데이터는 GE_ 에셋이 정하므로 이 클래스는 직접 걸 수 없다.
 */
UCLASS(Abstract)
class WXGAME_API UWxEffect_GuardReduction : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_GuardReduction();
};
