// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "WxDamageEffectContext.generated.h"

class UGameplayEffect;

/** 한 타격의 Damage GE에 피해 행의 추가 효과 목록을 넘기는 입력 Context. 서버 로컬이며 복제하지 않는다. */
USTRUCT()
struct WXCOMBAT_API FWxDamageEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FWxDamageEffectContext();
	explicit FWxDamageEffectContext(const FGameplayEffectContext& Context);

	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FGameplayEffectContext* Duplicate() const override;

	static FWxDamageEffectContext* Get(const FGameplayEffectContextHandle& Handle);

	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;
};

template<>
struct TStructOpsTypeTraits<FWxDamageEffectContext> : TStructOpsTypeTraitsBase2<FWxDamageEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
