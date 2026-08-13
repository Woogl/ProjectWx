// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemGlobals.h"
#include "Damage/WxCombatEffectContext.h"
#include "GameplayEffectTypes.h"

FGameplayEffectContext* UWxAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FWxCombatEffectContext();
}

void UWxAbilitySystemGlobals::InitGameplayCueParameters(FGameplayCueParameters& CueParameters, const FGameplayEffectContextHandle& EffectContext)
{
	Super::InitGameplayCueParameters(CueParameters, EffectContext);

	// 순정은 컨텍스트만 통째로 넘기고 Location은 비워 두므로, GE가 들고 다니는 큐는 여기서 채우지 않으면 원점에서 터진다.
	// GE 스펙과 RPC 스펙 양쪽 오버로드가 이 함수로 수렴하니 서버 멀티캐스트와 클라 예측 발행이 함께 덮인다.
	if (const FGameplayEffectContext* RawContext = EffectContext.Get())
	{
		if (const FHitResult* HitResult = RawContext->GetHitResult())
		{
			// 스윕의 Location은 형상 중심이라, 표면 접점인 ImpactPoint여야 임팩트 연출이 맞는 자리에 난다.
			CueParameters.Location = HitResult->ImpactPoint;
		}
	}
}
