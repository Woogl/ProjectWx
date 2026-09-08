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

	/**
	 * 비워 두면 이 공격은 피격 반응을 일으키지 않는다 — 무반응을 고르는 수단이라 기본 반응으로 대체하지 않는다.
	 * 패리·가드 브레이크는 전투 시스템이 별도 이벤트로 생성하므로 저작하지 않는다.
	 */
	UPROPERTY(EditAnywhere, meta = (Categories = "HitReact"))
	FGameplayTag HitReactTag;

	UPROPERTY(EditAnywhere)
	bool bCanCritical = true;

	/** false이면 이 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere)
	bool bCanGuard = true;

	/**
	 * false이면 퍼펙트 가드로 막아도 공격자가 역경직에 걸리지 않는다. 막아낸 대가인 GP 반사는 그대로 들어간다.
	 * 투사체는 true여도 공격자가 이 둘을 받지 않는다 — 되돌아가는 투사체가 곧 보복이다.
	 */
	UPROPERTY(EditAnywhere)
	bool bCanParry = true;

	/** Damage GE가 적용되고 퍼펙트 가드가 아니면 타겟에 추가 적용된다. */
	UPROPERTY(EditAnywhere, meta = (AllowAbstract = "false"))
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEffects;

    TArray<FGameplayEffectSpecHandle> MakeSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const;
};
