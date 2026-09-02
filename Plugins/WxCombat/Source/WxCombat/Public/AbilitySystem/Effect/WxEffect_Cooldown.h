// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxEffect_Cooldown.generated.h"

struct FGameplayTag;

/**
 * 충전형 쿨다운의 베이스.
 * 지속시간은 UWxMMC_CooldownDuration이 소스 어빌리티의 테이블 행에서 읽으므로, 엔진 순정 ApplyCooldown/CheckCooldown/쿨다운 조회 API를 그대로 쓴다.
 *
 * 소모한 충전 하나가 스택 하나다.
 * 스택이 더 쌓여도 진행 중인 회복은 건드리지 않고(NeverRefresh), 만료마다 스택 하나만 돌려주며 다음 회복을 시작한다(RemoveSingleStackAndRefreshDuration) — 충전이 직렬로 돌아온다.
 * 충전 상한은 GE가 아니라 테이블의 MaxRecharges이며 UWxAbilityBase::CheckCooldown이 판정한다.
 *
 * 엔진은 스택을 GE 클래스 단위로 병합한다.
 * 그래서 쿨다운을 따로 굴리는 어빌리티마다 파생 클래스를 하나씩 두고, 어빌리티가 CooldownGameplayEffectClass로 자기 것을 지정한다.
 * 파생 클래스가 부여하는 태그가 순정 쿨다운 API의 식별자다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();

protected:
	void GrantCooldownTag(const FGameplayTag& CooldownTag);
};

UCLASS()
class WXCOMBAT_API UWxMMC_CooldownDuration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

/** 이름은 그 어빌리티의 식별 태그 Ability.X를 따른다. */

UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown_Dodge : public UWxEffect_Cooldown
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown_Dodge();
};

UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown_Skill_1 : public UWxEffect_Cooldown
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown_Skill_1();
};

UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown_Ultimate : public UWxEffect_Cooldown
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown_Ultimate();
};
