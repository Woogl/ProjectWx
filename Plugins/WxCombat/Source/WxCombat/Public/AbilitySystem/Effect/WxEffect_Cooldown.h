// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * Duration은 어빌리티가 SetByCaller.Duration으로 실어 보낸다 — 직렬 회복분까지 더한 최종값이다.
 * 엔진이 적용 도중 지속시간을 여러 번 다시 계산하므로, 그때마다 같은 값을 되읽도록 라이브 상태에 기대지 않는다.
 * 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 * 소모된 충전 1개당 GE 1개가 활성 상태가 되며, GE 만료 = 충전 1개 회복이다.
 *
 * 아울러 어빌리티 CooldownGameplayEffectClass의 "프로젝트 방식(테이블 기반)" 기본 마커를 겸한다.
 * 이 클래스면 프로젝트 경로, 다른 GE면 커스텀 경로다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
