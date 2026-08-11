// Copyright Woogle. All Rights Reserved.

#include "Damage/WxCombatEffectContext.h"

UScriptStruct* FWxCombatEffectContext::GetScriptStruct() const
{
	return StaticStruct();
}

FGameplayEffectContext* FWxCombatEffectContext::Duplicate() const
{
	FWxCombatEffectContext* NewContext = new FWxCombatEffectContext();
	*NewContext = *this;

	// 순정 구현과 동일하게 히트 결과만 깊은 복사한다.
	if (GetHitResult())
	{
		NewContext->AddHitResult(*GetHitResult(), true);
	}

	return NewContext;
}

bool FWxCombatEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// 이 컨텍스트는 Cue 파라미터에 실려 클라이언트까지 간다 — Cue가 판정 결과를 읽을 수 있어야 한다.
	uint8 SerializedResult = static_cast<uint8>(DamageResult);
	uint8 SerializedCritical = bCritical ? 1 : 0;
	Ar << SerializedResult;
	Ar << SerializedCritical;
	Ar << FinalDamage;
	HitReactTag.NetSerialize(Ar, Map, bOutSuccess);

	if (Ar.IsLoading())
	{
		DamageResult = static_cast<EWxDamageResult>(SerializedResult);
		bCritical = SerializedCritical != 0;
	}

	bOutSuccess = true;
	return true;
}

EWxDamageResult FWxCombatEffectContext::GetDamageResult() const
{
	return DamageResult;
}

bool FWxCombatEffectContext::IsCritical() const
{
	return bCritical;
}

float FWxCombatEffectContext::GetFinalDamage() const
{
	return FinalDamage;
}

const FGameplayTag& FWxCombatEffectContext::GetHitReactTag() const
{
	return HitReactTag;
}

void FWxCombatEffectContext::ClearDamageResult()
{
	DamageResult = EWxDamageResult::None;
	bCritical = false;
	FinalDamage = 0.f;
	HitReactTag = FGameplayTag();
}

void FWxCombatEffectContext::SetEvaded()
{
	DamageResult = EWxDamageResult::Evaded;
}

void FWxCombatEffectContext::SetPerfectGuard(float ReflectAmount)
{
	DamageResult = EWxDamageResult::PerfectGuard;
	FinalDamage = ReflectAmount;
}

void FWxCombatEffectContext::SetDamaged(float InFinalDamage, bool bIsCritical, const FGameplayTag& InHitReactTag)
{
	DamageResult = EWxDamageResult::Damaged;
	FinalDamage = InFinalDamage;
	bCritical = bIsCritical;
	HitReactTag = InHitReactTag;
}
