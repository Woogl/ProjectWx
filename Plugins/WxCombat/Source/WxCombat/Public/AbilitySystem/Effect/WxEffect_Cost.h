// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cost.generated.h"

/**
 * 어빌리티 공용 코스트 GameplayEffect.
 *
 * Instant 정책이며, MP·UP 두 모디파이어를 정적으로 선언한다.
 * 값은 각 MMC(UWxMMC_MPCost/UWxMMC_UPCost)가 계산 시점에 소스 어빌리티(UWxAbilityBase)의 AbilityDataRow에서 MPCost/UPCost를 조회해 음수로 반환한다.
 * CDO가 완전 자기완결이라 엔진 순정 CheckCost(CanApplyAttributeModifiers)/ApplyCost/GetCostGameplayEffect를 그대로 사용한다(어빌리티 오버라이드 불필요).
 *
 * 아울러 어빌리티 CostGameplayEffectClass의 "프로젝트 방식(테이블 기반)" 기본 클래스를 겸한다.
 * 이 클래스면 테이블 기반, 다른 GE면 그 GE를 순정 경로로 사용한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cost();
};
