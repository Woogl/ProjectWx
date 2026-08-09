// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "WxDamageInfo.h"
#include "WxGameplayTags.h"

bool UWxCombatLibrary::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration)
{
	if (!Source || !Target)
	{
		return false;
	}

	AActor* SourceActor = Source->GetOwnerActor();
	const UGameplayAbility* AnimatingAbility = Source->GetAnimatingAbility();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddInstigator(SourceActor, SourceActor);
	Context.SetAbility(AnimatingAbility);
	Context.AddHitResult(HitResult);

	// 애님 노티파이는 어빌리티 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	// 대신 몽타주를 재생 중인 어빌리티의 활성화 키를 쓰면, 클라는 자기가 만든 키를 서버는 그 클라가 보낸 같은 키를 들고 있어 양쪽 적용이 correlate된다.
	// 시뮬 프록시는 복제 몽타주라 AnimatingAbility가 없어 키가 무효로 남고 엔진 권위 검사에서 걸러진다.
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			// 스펙에 실어 보내기만 하고, 실제 발동은 ExecCalc가 적중 판정 뒤에 처리한다.
			if (HitStopDuration > 0.f)
			{
				Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_HitStop, HitStopDuration);
			}

			const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, PredictionKey);
			bAppliedAny |= AppliedHandle.WasSuccessfullyApplied();
		}
	}

	return bAppliedAny;
}
