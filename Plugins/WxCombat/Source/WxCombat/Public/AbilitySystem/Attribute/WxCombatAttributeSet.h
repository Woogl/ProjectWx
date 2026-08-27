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

	/** 가드 피격으로 소모되며, 0이 되면 GuardBreak */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_SP)
	FGameplayAttributeData SP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, SP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxSP)
	FGameplayAttributeData MaxSP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxSP)

	/** MaxDP에 도달하면 그로기 발동 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_DP)
	FGameplayAttributeData DP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, DP)

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Vital", ReplicatedUsing = OnRep_MaxDP)
	FGameplayAttributeData MaxDP;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, MaxDP)

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

	/** ExecCalc가 최종 데미지를 실어 보내면 PostGameplayEffectExecute가 HP로 옮긴다. 직접 수정 금지. */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UWxCombatAttributeSet, IncomingDamage)

	/**
	 * 퍼펙트 가드로 막아낸 히트에서 공격자에게 되돌려줄 양. 직접 수정 금지.
	 *
	 * 퍼펙트 가드는 대상 어트리뷰트를 하나도 바꾸지 않아 이 통로가 없으면 PostGameplayEffectExecute가 아예 돌지 않는다.
	 */
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

private:
	/**
	 * 어트리뷰트 값의 허용 범위를 강제한다 — 하한 0, 짝 최대치가 있는 자원은 그 값이 상한.
	 *
	 * Current와 Base 양쪽 Pre 훅이 같은 규칙을 쓰도록 한 곳에 모았다.
	 * 최대치가 아직 0인 초기화 도중에는 상한을 걸지 않는다 — 짝이 채워지기 전에 자원이 0으로 눌린다.
	 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	/**
	 * 적중이 확정된 뒤 그 히트의 판정 결과를 소비한다 — 가드 해제, 피격 이벤트(반응 종류 동봉), 대미지 플로터.
	 *
	 * 판정은 UWxExecCalc_Damage가 스펙 태그와 FWxCombatEffectContext에 남긴 것을 그대로 쓴다.
	 * IncomingDamage가 ExecCalc 출력의 맨 뒤라서, 여기 닿을 때는 DP까지 확정돼 그로기 진입이 반응 라우팅에 보인다.
	 */
	void ProcessDamageTaken(const FGameplayEffectModCallbackData& Data, float Damage);

	/**
	 * 퍼펙트 가드로 막아낸 히트의 후속 — 공격자에게 반사 DP, 가드자에 퍼펙트 가드 이벤트, 공격자에 패리 반동 피격 이벤트, 큐.
	 *
	 * 공격자에게 돌아가는 두 갈래(반사 DP·패리 반동)는 공격이 Damage.CanParry를 달았을 때만 나간다.
	 * 가드자가 보는 이벤트와 큐는 반사량이 0이어도 나간다 — 막아낸 사실 자체는 늘 알려야 한다.
	 */
	void ProcessPerfectGuard(const FGameplayEffectModCallbackData& Data, float ReflectAmount);
};
