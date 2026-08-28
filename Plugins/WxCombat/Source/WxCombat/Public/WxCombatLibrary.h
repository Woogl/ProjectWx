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
	/** 적대 관계일 때만 true. 어느 쪽이든 팀이 없으면 적대가 아니다. */
	static bool IsHostile(const AActor* Source, const AActor* Target);

	/**
	 * 모든 대미지 경로의 단일 진입점.
	 * 성립하지 않는 히트(아군·시체·회피)는 GE를 걸지 않고, 회피만 여기서 DodgeSuccess 이벤트를 낸다.
	 *
	 * @param Causer			히트를 낸 액터. ASC가 없으면 Owner가 공격자다.
	 * @param HitStopDuration	0보다 크면 적중 시 공격자에 히트스톱.
	 * @return					대미지 GE가 적용됐으면 true. 회피(DodgeSuccess)·적용 실패는 false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration = 0.f);

	/**
	 * 구간형 상태 GE(무적·퍼펙트가드 등)를 건다.
	 * @param PredictingAbility	이 어빌리티의 예측 키로 걸어 소유 클라도 같은 프레임에 태그를 갖는다.
	 */
	static void ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility);
	
	/**
	 * 런타임에 Instant Gameplay Effect를 동적 생성하여 어트리뷰트 변경을 적용한다.
	 */
	static void ModifyAttribute(UAbilitySystemComponent* TargetASC, const FGameplayAttribute& Attribute, float Delta);
};
