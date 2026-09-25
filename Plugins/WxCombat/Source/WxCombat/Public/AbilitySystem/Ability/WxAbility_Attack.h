// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Attack.generated.h"

/**
 * 행 몽타주의 첫 단계 섹션을 재생하고, 콤보 창 구간의 재발동이 다음 단으로 넘긴다(터미널 단에서는 첫 단으로 되돌아간다).
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)이라 단계마다 CommitAbility가 새로 걸린다.
 *
 * 공격 종류마다 캐릭터 공통 발동 조건이 달라 아래 파생 타입으로 나눈다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Attack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void HandleMontageCompleted() override;

	virtual void OnComboWindowClosed() override;

	/** 파생 타입이 자기 식별 태그를 에셋 태그와 소유 태그에 건다. */
	void SetAttackTag(const FGameplayTag& AttackTag);

private:
	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다. */
	int32 ComboIndex = INDEX_NONE;
};

/** 공중·회피 중에는 나가지 않는다. */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack_Light : public UWxAbility_Attack
{
	GENERATED_BODY()

public:
	UWxAbility_Attack_Light();
};

/** 약공격을 끊고 들어간다. 공중·회피 중에는 나가지 않는다. */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack_Heavy : public UWxAbility_Attack
{
	GENERATED_BODY()

public:
	UWxAbility_Attack_Heavy();
};

/** 공중에서만 나간다. */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack_Air : public UWxAbility_Attack
{
	GENERATED_BODY()

public:
	UWxAbility_Attack_Air();
};

/** 회피 중에만 나간다. 진입 시점은 회피 몽타주의 StartRecovery가 차단을 푸는 때다. */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack_DodgeCounter : public UWxAbility_Attack
{
	GENERATED_BODY()

public:
	UWxAbility_Attack_DodgeCounter();
};
