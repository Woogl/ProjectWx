// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_AddAttribute.generated.h"

class UAbilitySystemComponent;

/**
 * 어트리뷰트 하나를 즉시 가감하는 GE의 베이스. 증감량은 SetByCaller.Magnitude 로 실어 보낸다.
 * 파생 클래스는 생성자에서 AddAttributeModifier로 대상 어트리뷰트를 지정하고, Apply를 공개한다.
 *
 * 어트리뷰트마다 파생 클래스를 두는 이유: 모디파이어를 한 GE에 모으면 적용 시 매그니튜드가 0인 모디파이어도
 * 건너뛰지 않아, 값을 싣지 않은 어트리뷰트까지 PostGameplayEffectExecute가 돈다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_AddAttribute : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_AddAttribute();

protected:
	void AddAttributeModifier(const FGameplayAttribute& Attribute);

	/** 공용 구현. static은 어느 파생 클래스로 불렸는지 알 수 없어, 파생 클래스의 Apply가 자기 클래스를 실어 부른다. */
	static void Apply(TSubclassOf<UWxEffect_AddAttribute> EffectClass, UAbilitySystemComponent* TargetASC, float Delta, const FGameplayEffectContextHandle& Context);
};

/**
 * 이름은 대상 어트리뷰트를 따른다. Delta에 음수를 넣으면 차감된다.
 * Context는 이 변경의 원인을 가리킨다 — 비워 두면 TargetASC 자신이 원인이 된다.
 */

UCLASS()
class WXCOMBAT_API UWxEffect_AddGP : public UWxEffect_AddAttribute
{
	GENERATED_BODY()

public:
	UWxEffect_AddGP();

	/** MaxGP 도달 시 그로기 판정은 AttributeSet이 이어받는다. */
	static void Apply(UAbilitySystemComponent* TargetASC, float Delta, const FGameplayEffectContextHandle& Context = FGameplayEffectContextHandle());
};

/** ATK·DEF를 타지 않는 고정량 피해. 공격이 낸 피해는 WxEffect_Damage(ExecCalc 경유)를 쓴다. */
UCLASS()
class WXCOMBAT_API UWxEffect_AddIncomingDamage : public UWxEffect_AddAttribute
{
	GENERATED_BODY()

public:
	UWxEffect_AddIncomingDamage();

	/** 사망 판정까지 AttributeSet의 IncomingDamage 소비 단계가 처리한다. */
	static void Apply(UAbilitySystemComponent* TargetASC, float Delta, const FGameplayEffectContextHandle& Context = FGameplayEffectContextHandle());
};
