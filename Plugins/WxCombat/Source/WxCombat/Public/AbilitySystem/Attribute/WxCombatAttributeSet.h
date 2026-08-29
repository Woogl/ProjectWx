// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WxCombatAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 약어 정의 (Max~ 는 각각의 상한값)
 *   HP                 - Health Points
 *   SP                 - Stamina Points
 *   GP                 - Groggy Points
 *   MP                 - Mana Points
 *   UP                 - Ultimate Points
 *   ATK / DEF          - Attack / Defense
 *   CritRate / CritDMG - Critical Rate / Critical Damage
 *   SPD / ASPD         - Speed / Attack Speed
 */
UCLASS()
class WXCOMBAT_API UWxCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UWxCombatAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** 0이 되면 사망 처리 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_HP)
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, HP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxHP)
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxHP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_SP)
	FGameplayAttributeData SP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, SP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxSP)
	FGameplayAttributeData MaxSP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxSP)

	/** MaxGP에 도달하면 그로기 발동 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_GP)
	FGameplayAttributeData GP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, GP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxGP)
	FGameplayAttributeData MaxGP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxGP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MP)
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MaxMP)
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxMP)
	
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_UP)
	FGameplayAttributeData UP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, UP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MaxUP)
	FGameplayAttributeData MaxUP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxUP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_ATK)
	FGameplayAttributeData ATK;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, ATK)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_DEF)
	FGameplayAttributeData DEF;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, DEF)

	/** 치명타 확률(1당 1%) */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_CritRate)
	FGameplayAttributeData CritRate;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, CritRate)

	/** 치명타 추가 피해(1당 1%) */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_CritDMG)
	FGameplayAttributeData CritDMG;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, CritDMG)
	
	/** MaxWalkSpeed에 곱해지는 이동 속도 배율(기본 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_SPD)
	FGameplayAttributeData SPD;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, SPD)

	/** 어빌리티 몽타주 PlayRate에 곱해지는 공격 속도 배율(기본 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_ASPD)
	FGameplayAttributeData ASPD;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, ASPD)

	// ── Meta (복제 안 함) ──────────────────────────────────────────────────

	/** ExecCalc가 최종 데미지를 실어 보내면 PostGameplayEffectExecute가 HP로 옮긴다. */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, IncomingDamage)

	/** 퍼펙트 가드는 대상 어트리뷰트를 바꾸지 않아, PostGameplayEffectExecute 실행을 위해 반사량을 이 통로로 전달한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Meta")
	FGameplayAttributeData IncomingReflect;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, IncomingReflect)

protected:
	UFUNCTION()
	void OnRep_HP(const FGameplayAttributeData& OldHP);

	UFUNCTION()
	void OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP);

	UFUNCTION()
	void OnRep_SP(const FGameplayAttributeData& OldSP);

	UFUNCTION()
	void OnRep_MaxSP(const FGameplayAttributeData& OldMaxSP);

	UFUNCTION()
	void OnRep_GP(const FGameplayAttributeData& OldGP);

	UFUNCTION()
	void OnRep_MaxGP(const FGameplayAttributeData& OldMaxGP);

	UFUNCTION()
	void OnRep_MP(const FGameplayAttributeData& OldMP);

	UFUNCTION()
	void OnRep_MaxMP(const FGameplayAttributeData& OldMaxMP);

	UFUNCTION()
	void OnRep_UP(const FGameplayAttributeData& OldUP);

	UFUNCTION()
	void OnRep_MaxUP(const FGameplayAttributeData& OldMaxUP);

	UFUNCTION()
	void OnRep_ATK(const FGameplayAttributeData& OldATK);

	UFUNCTION()
	void OnRep_DEF(const FGameplayAttributeData& OldDEF);

	UFUNCTION()
	void OnRep_CritRate(const FGameplayAttributeData& OldCritRate);

	UFUNCTION()
	void OnRep_CritDMG(const FGameplayAttributeData& OldCritDMG);

	UFUNCTION()
	void OnRep_SPD(const FGameplayAttributeData& OldSPD);

	UFUNCTION()
	void OnRep_ASPD(const FGameplayAttributeData& OldASPD);

private:
	struct FMaxAttributePair
	{
		FGameplayAttribute Attribute;
		FGameplayAttribute MaxAttribute;
	};

	static const FMaxAttributePair* FindAttributeMaxPair(const FGameplayAttribute& Attribute);
	float ClampAttributeValue(const FGameplayAttribute& Attribute, float NewValue) const;
	void AdjustCurrentAttributeForMaxChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue);
	
	void ProcessDamageTaken(const FGameplayEffectModCallbackData& Data, float Damage);
	void ProcessPerfectGuard(const FGameplayEffectModCallbackData& Data, float ReflectAmount);
};
