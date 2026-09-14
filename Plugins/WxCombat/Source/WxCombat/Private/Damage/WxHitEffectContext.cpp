// Copyright Woogle. All Rights Reserved.

#include "Damage/WxHitEffectContext.h"
#include "Engine/DataTable.h"
#include "Engine/PackageMapClient.h"

FWxHitEffectContext::FWxHitEffectContext() = default;

FWxHitEffectContext::FWxHitEffectContext(const FGameplayEffectContext& Context, const FDataTableRowHandle& Row)
	: FGameplayEffectContext(Context), DamageTable(const_cast<UDataTable*>(Row.DataTable.Get())), DamageRowName(Row.RowName)
{
}

UScriptStruct* FWxHitEffectContext::GetScriptStruct() const
{
	return StaticStruct();
}

FGameplayEffectContext* FWxHitEffectContext::Duplicate() const
{
	FWxHitEffectContext* Copy = new FWxHitEffectContext(*this);
	Copy->ResetDamageResult();
	if (GetHitResult())
	{
		Copy->AddHitResult(*GetHitResult(), true);
	}
	return Copy;
}

bool FWxHitEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	const bool bBaseMapped = FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
	UObject* Table = DamageTable.Get();
	const bool bTableMapped = Map->SerializeObject(Ar, UDataTable::StaticClass(), Table);
	Ar << DamageRowName;
	uint8 DefenseBits = (bGuarded ? 1 : 0) | (bPerfectGuard ? 2 : 0);
	Ar.SerializeBits(&DefenseBits, 2);
	if (Ar.IsLoading())
	{
		ResetDamageResult();
		DamageTable = Cast<UDataTable>(Table);
		bGuarded = (DefenseBits & 1) != 0;
		bPerfectGuard = (DefenseBits & 2) != 0;
	}
	bOutSuccess = bOutSuccess && !Ar.IsError();
	return bBaseMapped && bTableMapped;
}

void FWxHitEffectContext::ResetDamageResult()
{
	bDamageApplied = false;
	DamageMagnitude = 0.f;
	ReflectMagnitude = 0.f;
	bHasReflect = false;
	DamageResultTags.Reset();
	DamageTargetTags.Reset();
}

FWxHitEffectContext* FWxHitEffectContext::Get(const FGameplayEffectContextHandle& Handle)
{
	const FGameplayEffectContext* Context = Handle.Get();
	if (!Context || !Context->GetScriptStruct()->IsChildOf(StaticStruct()))
	{
		return nullptr;
	}
	// GAS의 공유 Context에 방어 판정과 로컬 적용 결과를 기록한다.
	return static_cast<FWxHitEffectContext*>(const_cast<FGameplayEffectContext*>(Context));
}
