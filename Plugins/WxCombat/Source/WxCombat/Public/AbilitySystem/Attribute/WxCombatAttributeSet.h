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
 * 캐릭터 스탯 어트리뷰트 세트.
 *
 * 약어 정의 (Max~ 는 각각의 상한값)
 *   HP                 - Health Points
 *   SP                 - Stamina Points
 *   DP                 - Daze Points
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
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// ── Vital ──────────────────────────────────────────────────────────────

	/** 0이 되면 사망 처리 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_HP)
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, HP)

	/** 최대 체력 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxHP)
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxHP)

	/** 가드 피격으로 소모되며, 0이 되면 GuardBreak */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_SP)
	FGameplayAttributeData SP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, SP)

	/** 최대 스태미나 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxSP)
	FGameplayAttributeData MaxSP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxSP)

	/** MaxDP에 도달하면 그로기 발동 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_DP)
	FGameplayAttributeData DP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, DP)

	/** 최대 그로기 수치 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxDP)
	FGameplayAttributeData MaxDP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxDP)

	// ── Resource ────────────────────────────────────────────────────────────
	
	/** 스킬 사용 비용으로 소모하는 마나 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MP)
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MP)

	/** 최대 마나 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MaxMP)
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxMP)
	
	/** 궁극기 사용 비용으로 소모하는 수치 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_UP)
	FGameplayAttributeData UP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, UP)

	/** 최대 궁극기 수치 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MaxUP)
	FGameplayAttributeData MaxUP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxUP)

	// ── Combat ─────────────────────────────────────────────────────────────

	/** 데미지 계산의 기반 수치 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_ATK)
	FGameplayAttributeData ATK;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, ATK)

	/** 데미지 감소 계산에 쓰는 방어력 */
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

	/** ExecCalc가 최종 데미지를 실어 보내면 PostGameplayEffectExecute가 HP로 옮긴다. 직접 수정 금지. */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, IncomingDamage)

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
	void OnRep_DP(const FGameplayAttributeData& OldDP);

	UFUNCTION()
	void OnRep_MaxDP(const FGameplayAttributeData& OldMaxDP);

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
};
