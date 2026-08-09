// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "WxDamageInfo.generated.h"

class UAbilitySystemComponent;
struct FWxDamageTableRow;
struct FDataTableRowHandle;

/**
 * 대미지 한 건의 설계 데이터.
 * AnimNotify에서 편집돼 무기·투사체로 전달되며, Damage GE Spec을 만들 때 SetByCaller와 DynamicAssetTags로 변환된다.
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxDamageInfo
{
	GENERATED_BODY()

	FWxDamageInfo();

	/** RowHandle이 가리키는 대미지 테이블 행으로 FWxDamageInfo를 만든다. */
	static FWxDamageInfo FromDataRow(const FDataTableRowHandle& RowHandle);

	/** 첫 항목은 UWxEffect_Damage Spec이고, 이후는 AdditionalEffectClasses 각각의 Spec이다. */
	TArray<FGameplayEffectSpecHandle> MakeSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const;

	/** 공격력 계수. Damage Spec의 SetByCaller.Coeff.ATK로 반영 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo")
	float CoeffATK = 1.f;

	/** 적중 시 공격자 MP 회복량. Damage Spec의 SetByCaller.Recovery.MP로 반영 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo")
	float RecoverMP = 0.f;

	/** 적중 시 공격자 UP 회복량. Damage Spec의 SetByCaller.Recovery.UP로 반영 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo")
	float RecoverUP = 0.f;

	/**
	 * 적중 시 부여할 HitReact 태그.
	 * 비어있으면 HitReact 이벤트가 송출되지 않는다.
	 * Event.HitReact.Normal/Knockback/Knockdown/Knockup 등을 지정하면 해당 종류의 HitReact가 발동된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo", meta = (Categories = "Event.HitReact"))
	FGameplayTag HitReactTag;

	/** true이면 이 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo")
	bool bUnblockable = false;

	/** true이면 퍼펙트 가드 성공 시 공격자에게 HitReact를 발동시킨다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo")
	bool bParryHitReact = true;

	/** Damage GE와 함께 타겟에 적용할 추가 GameplayEffect 목록 (상태이상, 디버프 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|DamageInfo", meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;
};
