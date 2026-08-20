// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "WxCombatEffectContext.generated.h"

UENUM()
enum class EWxDamageResult : uint8
{
	/** ExecCalc가 돌지 않았거나 판정 전에 빠져나갔다(팀 등) — 연출 대상이 아니다 */
	None,

	/** 무적으로 무효화 */
	Evaded,

	/** 일반 가드로 감소된 경우를 포함 */
	Damaged
};

/**
 * 치명타 여부를 실어 나르는 EffectContext.
 *
 * UWxExecCalc_Damage가 판정 중에 적고, 어트리뷰트가 확정된 뒤 UWxCombatAttributeSet::ProcessDamageTaken가 읽어 플로터에 넘긴다.
 * 나머지 판정 결과는 스펙 태그와 IncomingDamage로 복원되지만 크리 여부만은 남는 흔적이 없어 이 통로가 필요하다.
 *
 * 하나의 컨텍스트가 대미지 GE와 AdditionalEffects 전체에 공유되므로(FWxDamageInfo::MakeSpecs), 판정을 새로 시작할 때 먼저 지워야 한다.
 *
 * 할당은 UWxAbilitySystemGlobals가 맡으므로 MakeEffectContext를 타는 모든 GE가 이 타입을 갖는다.
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

	/** 컨텍스트가 여러 스펙에 재사용되므로, 판정을 새로 시작할 때도 false로 한 번 불러 이전 결과를 지운다 */
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
