// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "WxDamageTableRow.generated.h"

USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxDamageTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float CoeffATK = 1.f;

	/** 공격이 요청하는 피격 반응. 패리·가드 브레이크는 전투 시스템이 별도 이벤트로 생성하므로 저작하지 않는다. */
	UPROPERTY(EditAnywhere, meta = (Categories = "HitReact"))
	FGameplayTag HitReactTag;

	UPROPERTY(EditAnywhere)
	bool bCanCritical = true;

	/** false이면 이 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere)
	bool bCanGuard = true;

	/** false이면 퍼펙트 가드로 막아도 패리가 성립하지 않아 공격자가 GP 반사도 역경직도 받지 않는다 */
	UPROPERTY(EditAnywhere)
	bool bCanParry = true;

	/** Damage GE가 적용되고 퍼펙트 가드가 아니면 타겟에 추가 적용된다. */
	UPROPERTY(EditAnywhere, meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;

    TArray<FGameplayEffectSpecHandle> MakeSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const;
};
