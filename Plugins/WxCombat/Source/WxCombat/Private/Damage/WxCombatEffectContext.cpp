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
	const bool bParentSucceeded = FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
	
	uint8 SerializedCritical = bCritical ? 1 : 0;
	Ar << SerializedCritical;

	if (Ar.IsLoading())
	{
		bCritical = SerializedCritical != 0;
	}

	return bParentSucceeded;
}

bool FWxCombatEffectContext::IsCritical() const
{
	return bCritical;
}

void FWxCombatEffectContext::SetCritical(bool bInCritical)
{
	bCritical = bInCritical;
}
