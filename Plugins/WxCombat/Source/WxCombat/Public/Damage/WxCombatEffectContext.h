// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "WxCombatEffectContext.generated.h"

/**
 * 치명타 여부를 실어 나르는 EffectContext.
 * UWxAbilitySystemGlobals가 할당하므로 MakeEffectContext를 타는 모든 GE가 이 타입을 갖는다.
 *
 * UWxExecCalc_Damage가 판정 중에 적고, 어트리뷰트가 확정된 뒤 UWxCombatAttributeSet::ProcessDamageTaken가 읽어 플로터에 넘긴다.
 * 나머지 판정 결과는 스펙 태그와 IncomingDamage로 복원되지만 크리 여부만은 남는 흔적이 없어 이 통로가 필요하다.
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
