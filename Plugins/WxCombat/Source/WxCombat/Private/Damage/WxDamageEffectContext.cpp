// Copyright Woogle. All Rights Reserved.

#include "Damage/WxDamageEffectContext.h"

FWxDamageEffectContext::FWxDamageEffectContext() = default;

FWxDamageEffectContext::FWxDamageEffectContext(const FGameplayEffectContext& Context)
	: FGameplayEffectContext(Context)
{
}

UScriptStruct* FWxDamageEffectContext::GetScriptStruct() const
{
	return StaticStruct();
}

FGameplayEffectContext* FWxDamageEffectContext::Duplicate() const
{
	FWxDamageEffectContext* Copy = new FWxDamageEffectContext(*this);
	if (GetHitResult())
	{
		Copy->AddHitResult(*GetHitResult(), true);
	}
	return Copy;
}

FWxDamageEffectContext* FWxDamageEffectContext::Get(const FGameplayEffectContextHandle& Handle)
{
	const FGameplayEffectContext* Context = Handle.Get();
	if (!Context || !Context->GetScriptStruct()->IsChildOf(StaticStruct()))
	{
		return nullptr;
	}
	return static_cast<FWxDamageEffectContext*>(const_cast<FGameplayEffectContext*>(Context));
}
