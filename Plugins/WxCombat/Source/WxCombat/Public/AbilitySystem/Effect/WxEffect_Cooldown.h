// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * 공용 쿨다운 GameplayEffect.
 *
 * 모든 어빌리티가 공유하는 단일 GE 클래스.
 * Duration은 UWxMMC_CooldownDuration MMC가 소스 어빌리티의 AbilityDataRow에서 CooldownTime을 조회하고 직렬 회복분을 더해 계산한다. 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 * 소모된 충전 1개당 GE 1개가 활성 상태가 되며, GE 만료 = 충전 1개 회복이다.
 *
 * 아울러 어빌리티 CooldownGameplayEffectClass의 "프로젝트 방식(테이블 기반)" 기본 마커를 겸한다. 이 클래스면 프로젝트 경로, 다른 GE면 커스텀 경로다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
