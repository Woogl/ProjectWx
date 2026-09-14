// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
struct FHitResult;

UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 어느 쪽이든 팀이 없으면 적대가 아니다. */
	static bool IsHostile(const AActor* Source, const AActor* Target);

	/**
	 * Hit Wrapper GE가 아군·시체를 거르고, 무적이면 피해 대신 DodgeSuccess 이벤트를 낸다.
	 * 서버에서만 적용하며 적중 연출과 추가 효과는 예측하지 않는다.
	 *
	 * @param Causer	히트를 낸 액터. ASC가 없으면 Owner가 공격자다.
	 * @return			대미지 GE가 적용됐으면 true. 비권위·회피(DodgeSuccess)·적용 실패는 false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult);

	/**
	 * 구간형 상태 GE(무적·퍼펙트가드 등)를 건다.
	 * @param PredictingAbility	이 어빌리티의 예측 키로 걸어 소유 클라도 같은 프레임에 태그를 갖는다.
	 */
	static void ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility);
};
