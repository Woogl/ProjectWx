// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Guard.generated.h"

/**
 * 방어 유효 상태(Effect.Guard)를 부여한다. 가드는 키를 쥔 동안 유지돼 고정 지속시간이 없으므로,
 * 수명은 가드 어빌리티가 끊는다 — 가드 종료와 SP 고갈로 가드가 깨질 때.
 *
 * 방어 판정의 수치도 여기 있다. ExecCalc가 Effect.Guard 태그를 본 뒤 이 감소율을 곱한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Guard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Guard();

	/** 어빌리티가 이 클래스를 그대로 부여하므로 적용된 인스턴스와 값이 같아 CDO에서 읽는다. */
	static float GetDamageReductionRate();

protected:
	/** 가드 중 받는 대미지 배율(0~1). 0.5면 50% 감소. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Guard", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageReductionRate = 0.5f;
};
