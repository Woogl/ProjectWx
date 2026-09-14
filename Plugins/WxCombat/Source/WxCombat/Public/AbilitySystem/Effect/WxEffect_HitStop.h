// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_HitStop.generated.h"

class UAbilitySystemComponent;

/**
 * 히트스톱(역경직). Effect.HitStop 태그를 SetByCaller.Duration만큼 부여하는 것이 전부다.
 *
 * 애니메이션과 이동을 세우고 되돌리는 건 UWxHitStopComponent가 이 GE의 추가·제거를 받아서 한다.
 * 중첩하지 않으므로 연타는 인스턴스가 따로 생기며, 클라이언트는 서버 GE의 복제를 따른다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_HitStop : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_HitStop();

	/**
	 * 서버에서 예측 키 없이 적용한다. 자기 자신에게 걸면 공격자 쪽 히트스톱이다.
	 * 엔진은 0 이하 지속시간을 0.1초로 올리며 에러를 남기므로, Duration이 0 이하면 걸지 않는다.
	 */
	static void Apply(float Duration, UAbilitySystemComponent* Source, UAbilitySystemComponent* Target);
};
