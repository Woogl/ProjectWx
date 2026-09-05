// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "WxCombatEffectContext.generated.h"

/**
 * UWxAbilitySystemGlobals가 할당해 MakeEffectContext 경로의 GE가 이 타입을 사용한다.
 * 어트리뷰트로 전달할 수 없는 크리 판정은 UWxExecCalc_Damage가 기록하고 UWxCombatAttributeSet::ProcessDamageTaken이 플로터에 전달한다.
 */
USTRUCT()
struct WXCOMBAT_API FWxCombatEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FGameplayEffectContext* Duplicate() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	bool IsCritical() const;

	void SetCritical(bool bInCritical);

protected:
	UPROPERTY()
	bool bCritical = false;
};

template<>
struct TStructOpsTypeTraits<FWxCombatEffectContext> : public TStructOpsTypeTraitsBase2<FWxCombatEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
