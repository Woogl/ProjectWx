// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "WxGameplayTags.h"
#include "Damage/WxDamageTableRow.h"

bool UWxCombatLibrary::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration)
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
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	// 적중 성립 여부는 대미지가 들어가기 전 상태로 가른다 — 이 히트로 죽은 대상은 아직 살아 있던 것으로 쳐야 마무리 일격에도 역경직이 걸린다.
	const EWxDamageResult DamageCheck = UWxExecCalc_Damage::CheckDamage(Source, Target);
	const bool bHitLands = HitStopDuration > 0.f && DamageCheck == EWxDamageResult::Damaged;

	// 무적 회피는 어트리뷰트를 하나도 바꾸지 않아 적용 뒤에는 알아낼 방법이 없다. 같은 선판정을 쓰는 여기서 알린다.
	// 대미지 GE와 AdditionalEffects 적용은 그대로 이어간다 — 회피해도 상태이상은 걸린다.
	if (DamageCheck == EWxDamageResult::Evaded)
	{
		AActor* TargetActor = Target->GetOwnerActor();

		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_DodgeSuccess, EventData);
	}
	
	FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(TEXT("GetDamageTableRow"));
	if (!DamageRow)
	{
		return false;
	}
	
	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageRow->MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, PredictionKey);
			bAppliedAny |= AppliedHandle.WasSuccessfullyApplied();
		}
	}

	// 발동만은 적용 뒤다.
	// GE 적용이 동기라 이 시점엔 피격자가 보낸 반응 이벤트(패리 등)가 도착해 있어, ApplyHitStop이 그 반응에 몽타주를 양보할 수 있다.
	if (bHitLands)
	{
		if (UWxAbilitySystemComponent* SourceWxASC = Cast<UWxAbilitySystemComponent>(Source))
		{
			SourceWxASC->ApplyHitStop(HitStopDuration, AnimatingAbility);
		}
	}

	return bAppliedAny;
}

FActiveGameplayEffectHandle UWxCombatLibrary::ApplyEffectForDuration(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Duration, const UGameplayAbility* PredictingAbility)
{
	if (!TargetASC || !EffectClass || Duration <= 0.f)
	{
		return FActiveGameplayEffectHandle();
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);

	// 잠그지 않으면 적용 단계에서 정의의 지속시간이 다시 실려 구간이 닫히지 않는다.
	Spec.SetDuration(Duration, true);

	// 애님 노티파이는 어빌리티 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (PredictingAbility)
	{
		PredictionKey = PredictingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	return TargetASC->ApplyGameplayEffectSpecToSelf(Spec, PredictionKey);
}
