// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "GameplayPrediction.h"
#include "WxAbility_Passive.generated.h"

class UGameplayEffect;

/**
 * 트리거 이벤트가 오면 지정한 효과를 자신에게 거는 패시브.
 *
 * 조립은 전부 에셋에서 한다 — 언제(순정 AbilityTriggers)와 무엇(TriggeredEffects)을 고르면 그것이 곧 새 패시브다.
 * 공격에 딸린 트리거(Event.DamageDealt)는 그 공격 1회 발동당 한 번만 지급한다. 광역·다단히트로 여러 번 적중해도 한 번이고, 콤보는 단마다 발동이 따로라 단마다 지급된다.
 * 트리거를 여럿 등록하면 그중 아무거나 왔을 때 효과 묶음 전체를 건다. 계기마다 다른 효과를 주려면 패시브를 나눈다.
 * 등록한 트리거끼리 조상 관계면 안 된다 — 부모와 자식을 같이 걸면 한 이벤트에 조상마다 발화해 효과가 두 번 걸린다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Passive : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Passive();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 트리거될 때마다 자신에게 걸고 손을 뗀다 — 어빌리티 수명에 묶여 종료에서 걷히는 ActivationOwnedEffects와 반대라, 지속형 효과도 그대로 남는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability")
	TArray<TSubclassOf<UGameplayEffect>> TriggeredEffects;

private:
	/** 이미 지급한 공격 발동의 예측 키. 같은 키로 또 들어온 적중은 그 발동에서 이미 받아 간 것이다. */
	FPredictionKey ChargedActivationKey;
};
