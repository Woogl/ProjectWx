// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "WxHitEffectContext.generated.h"

class UDataTable;
struct FDataTableRowHandle;

/** 타격 입력, 확정한 방어 판정과 로컬 적용 결과. */
USTRUCT()
struct WXCOMBAT_API FWxHitEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FWxHitEffectContext();
	FWxHitEffectContext(const FGameplayEffectContext& Context, const FDataTableRowHandle& Row);

	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FGameplayEffectContext* Duplicate() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	static FWxHitEffectContext* Get(const FGameplayEffectContextHandle& Handle);
	void ResetDamageResult();

	UPROPERTY()
	TWeakObjectPtr<UDataTable> DamageTable;

	UPROPERTY()
	FName DamageRowName;

	bool bGuarded = false;
	bool bPerfectGuard = false;

	/** ApplyDamage의 동기 반환용. 복제하지 않으며 Duplicate에서도 초기화한다. */
	bool bDamageApplied = false;

	// 실행 완료 후 Wrapper로 전달하는 로컬 결과. 복제와 Duplicate에서는 초기화한다.
	float DamageMagnitude = 0.f;
	float ReflectMagnitude = 0.f;
	bool bHasReflect = false;
	FGameplayTagContainer DamageResultTags;
	FGameplayTagContainer DamageTargetTags;
};

template<>
struct TStructOpsTypeTraits<FWxHitEffectContext> : TStructOpsTypeTraitsBase2<FWxHitEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
