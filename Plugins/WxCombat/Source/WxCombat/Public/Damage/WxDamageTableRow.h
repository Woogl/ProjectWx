// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "WxDamageTableRow.generated.h"

/**
 * 공격별 밸런스 수치(계수·피격 반응·판정 플래그)를 담는 데이터테이블 Row.
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxDamageTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float CoeffATK = 1.f;

	/** 비워 두면 부모 Event.Hit 평타로 나가 HitReact가 뜨지 않는다 */
	UPROPERTY(EditAnywhere, meta = (Categories = "Event.Hit"))
	FGameplayTag HitReactTag;

	UPROPERTY(EditAnywhere)
	bool bCanCritical = true;

	/** false이면 이 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere)
	bool bCanGuard = true;

	/** false이면 퍼펙트 가드로 막아도 패리가 성립하지 않아 공격자가 DP 반사도 역경직도 받지 않는다 */
	UPROPERTY(EditAnywhere)
	bool bCanParry = true;

	/** Damage GE와 함께 타겟에 적용된다 (상태이상, 디버프 등) */
	UPROPERTY(EditAnywhere, meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;

    TArray<FGameplayEffectSpecHandle> MakeSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const;
};
