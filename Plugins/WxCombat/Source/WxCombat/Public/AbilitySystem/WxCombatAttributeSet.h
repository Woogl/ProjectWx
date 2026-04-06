// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WxCombatAttributeSet.generated.h"

// 어트리뷰트 접근자 매크로
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 캐릭터 스탯 어트리뷰트 세트.
 *
 * 약어 정의
 *   HP       - Health Points      : 현재 체력
 *   MaxHP    - Max Health Points  : 최대 체력
 *   PP       - Poise Points       : 현재 강인도. 0이 되면 피격 경직(HitReact) 발동
 *   MaxPP    - Max Poise Points   : 최대 강인도
 *   DP       - Daze Points        : 현재 그로기 수치
 *   MaxDP    - Max Daze Points    : 최대 그로기 수치
 *   MP       - Mana Points        : 현재 마나
 *   MaxMP    - Max Mana Points    : 최대 마나
 *   UP       - Ultimate Points    : 현재 궁극기 수치
 *   MaxUP    - Max Ultimate Points: 최대 궁극기 수치
 *   ATK      - Attack             : 공격력
 *   DEF      - Defense            : 방어력
 *   CritRate - Critical Rate      : 치명타 확률 (1당 1%)
 *   CritDMG  - Critical Damage    : 치명타 피해 (1당 1% 추가 데미지)
 *   SPD      - Speed              : 이동 속도
 *
 * IncomingDamage: GameplayEffect ExecutionCalculation에서 최종 데미지를 이 어트리뷰트로 전달하고
 *                 PostGameplayEffectExecute에서 HP를 차감하는 패턴으로 사용.
 *                 
 * 복제됨: HP, MaxHP, PP, MaxPP, DP, MaxDP, MP, MaxMP, UP, MaxUP, ATK, DEF, CritRate, CritDMG, SPD
 * 복제 안 함 (Meta): IncomingDamage
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

	/** HP (Health Points) : 현재 체력. 0이 되면 사망 처리 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_HP)
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, HP)

	/** MaxHP (Max Health Points) : 최대 체력. HP의 상한값 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxHP)
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxHP)
	
	/** PP (Poise Points) : 리액션 없이 버틸 수 있는 한계치 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_PP)
	FGameplayAttributeData PP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, PP)

	/** MaxPP (Max Poise Points) : 최대 강인도. PP의 상한값 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxPP)
	FGameplayAttributeData MaxPP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxPP)

	/** DP (Daze Points) : 현재 그로기 수치. MaxDP에 도달하면 그로기 상태 발동 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_DP)
	FGameplayAttributeData DP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, DP)

	/** MaxDP (Max Daze Points) : 최대 그로기 수치. DP의 상한값 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxDP)
	FGameplayAttributeData MaxDP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxDP)

	// ── Resource ────────────────────────────────────────────────────────────
	
	/** MP (Mana Points) : 현재 마나. 스킬 사용 비용으로 소모 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MP)
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MP)

	/** MaxMP (Max Mana Points) : 최대 마나. MP의 상한값 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MaxMP)
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxMP)
	
	/** UP (Ultimate Points) : 현재 궁극기 수치. 궁극기 사용 비용으로 소모 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_UP)
	FGameplayAttributeData UP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, UP)

	/** MaxUP (Max Ultimate Points) : 최대 궁극기 수치. UP의 상한값 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Resource", ReplicatedUsing = OnRep_MaxUP)
	FGameplayAttributeData MaxUP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxUP)

	// ── Combat ─────────────────────────────────────────────────────────────

	/** ATK (Attack) : 공격력. 데미지 계산의 기반 수치 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_ATK)
	FGameplayAttributeData ATK;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, ATK)

	/** DEF (Defense) : 방어력. 데미지 감소 계산에 사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_DEF)
	FGameplayAttributeData DEF;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, DEF)

	/** CritRate (Critical Rate) : 치명타 확률. 1당 1% */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_CritRate)
	FGameplayAttributeData CritRate;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, CritRate)

	/** CritDMG (Critical Damage) : 치명타 추가 피해. 1당 1% 추가 데미지 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_CritDMG)
	FGameplayAttributeData CritDMG;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, CritDMG)
	
	/** SPD (Speed) : 이동 속도 배율. CharacterMovement의 MaxWalkSpeed에 곱해지는 Multiplier (기본값 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Combat", ReplicatedUsing = OnRep_SPD)
	FGameplayAttributeData SPD;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, SPD)

	// ── Meta (복제 안 함) ──────────────────────────────────────────────────

	/** IncomingDamage : 데미지 계산 전달용 메타 어트리뷰트. 직접 수정 금지 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, IncomingDamage)

protected:
	UFUNCTION()
	void OnRep_HP(const FGameplayAttributeData& OldHP);

	UFUNCTION()
	void OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP);

	UFUNCTION()
	void OnRep_PP(const FGameplayAttributeData& OldPP);

	UFUNCTION()
	void OnRep_MaxPP(const FGameplayAttributeData& OldMaxPP);

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
};
