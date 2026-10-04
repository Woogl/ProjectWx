// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "WxAbility_Passive.generated.h"

class UGameplayEffect;

/**
 * 트리거될 때마다 지정한 효과를 자신에게 거는 패시브.
 *
 * Event.DamageDealt 트리거는 대상마다 오므로, 여러 대상을 맞히면 그 수만큼 지급된다.
 * 트리거를 여럿 등록하면 그중 아무거나 왔을 때 효과 묶음 전체를 건다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_Passive : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Passive();

#if WITH_EDITOR
	/** 트리거끼리 조상 관계면 안 된다 — 부모와 자식을 같이 걸면 한 이벤트에 조상마다 발화해 효과가 두 번 걸린다. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 트리거될 때마다 자신에게 걸고 손을 뗀다 — 어빌리티 수명에 묶여 종료에서 걷히는 ActivationOwnedEffects와 반대라, 지속형 효과도 그대로 남는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability")
	TArray<TSubclassOf<UGameplayEffect>> TriggeredEffects;
};
