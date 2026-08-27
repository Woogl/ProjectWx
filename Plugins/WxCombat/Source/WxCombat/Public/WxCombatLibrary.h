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
	/**
	 * 판정이 성립하는 관계인지 가른다 — 적대에만 성립하며 아군·중립에는 대미지도 연출도 발생하지 않는다.
	 * 판정 근거가 없으면(둘 중 하나가 널이거나 공격자에게 팀 개념이 없으면) 성립으로 본다.
	 */
	static bool IsHostile(const AActor* Source, const AActor* Target);

	/**
	 * 모든 대미지 경로의 단일 진입점.
	 * 성립하지 않는 히트(아군·시체·회피)는 GE를 걸지 않고, 회피만 여기서 DodgeSuccess 이벤트를 낸다.
	 *
	 * 다만 대미지 GE는 Instant+Execution이라 엔진이 예측 시 execution을 건너뛴다.
	 * 어트리뷰트는 서버 권위로 남고, 실제로 예측되는 것은 DamageTableRow의 지속형 AdditionalEffects뿐이다.
	 *
	 * @param Causer			히트를 낸 액터. 컨텍스트의 원인 액터(EffectCauser)로 실려, 피격 반응이 이 액터를 공격이 온 방향으로 삼는다.
	 *							공격자는 여기서 역추적한다 — 이 액터가 ASC를 가지면 그것이 공격자이고(처형·맨손), 없으면 Owner가 공격자다(무기·투사체).
	 *							둘 다 아니면 공격자를 찾지 못해 아무것도 걸지 않는다.
	 * @param HitStopDuration	0보다 크고 적중이 성립하면 공격자 ASC의 히트스톱을 이 길이만큼 발동한다.
	 * @return					Spec 중 하나라도 권위 또는 예측으로 적용됐으면 true.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration = 0.f);

	/**
	 * 구간형 상태 GE(무적·퍼펙트가드 등)를 건다.
	 * @param PredictingAbility	이 어빌리티의 예측 키로 걸어 소유 클라도 같은 프레임에 태그를 갖는다.
	 */
	static void ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility);
};
