// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "WxDamageTableRow.generated.h"

/**
 * 대미지 수치 데이터테이블 Row 구조체.
 * 공격력 계수, 자원 회복, 피격 반응 등 공격별 밸런스 수치를 관리한다.
 * RowName 예시: Attack_Light_1, Attack_Heavy_2, Skill_Fireball
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxDamageTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 공격력 계수. Damage Spec의 SetByCaller.Coeff.ATK로 반영 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	float CoeffATK = 1.f;

	/** 적중 시 공격자 MP 회복량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	float RecoverMP = 0.f;

	/** 적중 시 공격자 UP 회복량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	float RecoverUP = 0.f;

	/**
	 * 적중 시 부여할 HitReact 태그.
	 * 기본값(Event.HitReact.Normal)은 PP 소진 조건일 때만 발동되고,
	 * Knockback/Knockdown/Knockup 등을 지정하면 PP 잔량과 무관하게 해당 종류의 HitReact가 강제 발동된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo", meta = (Categories = "Event.HitReact"))
	FGameplayTag HitReactTag;

	/** true이면 이 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo")
	bool bUnblockable = false;

	/** Damage GE와 함께 타겟에 적용할 추가 GameplayEffect 목록 (상태이상, 디버프 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageInfo", meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;
};
