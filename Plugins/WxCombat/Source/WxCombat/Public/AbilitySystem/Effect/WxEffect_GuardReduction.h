// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_GuardReduction.generated.h"

/**
 * 방어 유효 상태(Effect.GuardReduction)를 부여하고 경감률을 GuardReductionScale에 싣는다.
 * 가드는 키를 쥔 동안 유지돼 고정 지속시간이 없으므로, 수명은 가드 어빌리티가 끊는다 — 가드 종료와 SP 고갈로 가드가 깨질 때.
 *
 * 경감률 자체는 DT_Effect 행에서 온다. 행을 지목하는 Wx Effect Data 컴포넌트를 BP 자식이 붙이므로 이 클래스는 직접 걸 수 없다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_GuardReduction : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_GuardReduction();
};
