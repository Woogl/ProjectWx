// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "WxDamageTableRow.generated.h"

/**
 * 공격별 밸런스 수치(계수·자원 회복·피격 반응)를 담는 데이터테이블 Row.
 * RowName 예시: AM_Attack_L, AM_Attack_HH, AM_Finisher, BP_Projectile
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxDamageTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	float CoeffATK = 1.f;

	/** 적중 시 공격자 MP 회복량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	float RecoverMP = 0.f;

	/** 적중 시 공격자 UP 회복량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	float RecoverUP = 0.f;

	/** 비워 두면 HitReact 이벤트가 송출되지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo", meta = (Categories = "Event.HitReact"))
	FGameplayTag HitReactTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	bool bCanCritical = true;

	/** true이면 이 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	bool bUnblockable = false;

	/** true이면 퍼펙트 가드 성공 시 공격자에게 HitReact를 발동시킨다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	bool bParryHitReact = true;

	/** Damage GE와 함께 타겟에 적용된다 (상태이상, 디버프 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo", meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;
};
