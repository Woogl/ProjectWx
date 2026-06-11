// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * 공용 쿨다운 GameplayEffect.
 *
 * 모든 어빌리티가 공유하는 단일 GE 클래스.
 * Duration은 SetByCaller(SetByCaller.Duration)로 설정되며,
 * 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 * 소모된 충전 1개당 GE 1개가 활성 상태가 되며, GE 만료 = 충전 1개 회복이다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
