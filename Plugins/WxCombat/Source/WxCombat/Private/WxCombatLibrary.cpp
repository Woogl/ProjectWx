// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "WxDamageInfo.h"
#include "WxGameplayTags.h"

bool UWxCombatLibrary::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration)
{
	if (!Source || !Target)
	{
		return false;
	}

	AActor* SourceActor = Source->GetOwnerActor();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddInstigator(SourceActor, SourceActor);
	Context.SetAbility(Source->GetAnimatingAbility());
	Context.AddHitResult(HitResult);

	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			// 히트스톱 지속시간을 스펙에 실어 보낸다. 실제 발동(무적 회피 제외)은 WxExecCalc_Damage가 적중 판정 후 처리한다.
			if (HitStopDuration > 0.f)
			{
				Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_HitStop, HitStopDuration);
			}
			Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
			bAppliedAny = true;
		}
	}

	return bAppliedAny;
}
