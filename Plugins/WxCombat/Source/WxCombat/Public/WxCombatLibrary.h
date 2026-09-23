// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;

UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 어느 쪽이든 팀이 없으면 적대가 아니다. */
	static bool IsHostile(const AActor* Source, const AActor* Target);

	/**
	 * Causer 또는 그 Owner의 ASC로 피해를 적용한다. 투사체는 발사 레벨을, 나머지는 현재 Ability/레벨을 사용한다.
	 * 방어 판정·회피·반응·히트스톱·퍼펙트 가드 되돌림·추가 효과는 피해 GE가 처리한다.
	 *
	 * @return Damage GE 적용 여부. 양수 피해가 아니어도 true이며, 무적·사망·거부면 false다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult);

	/**
	 * 구간형 상태 GE(무적·퍼펙트가드 등)를 SourceAbility의 레벨(없으면 1)로 건다.
	 * 발동 창 안에서 부르면 그 발동의 예측 키로 걸려 소유 클라도 같은 프레임에 태그를 갖고, 창 밖(비동기 노티파이 등)에서는 키 없이 걸린다.
	 */
	static void ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* SourceAbility);
};
