// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
struct FWxDamageInfo;
struct FHitResult;

/** 무기·투사체에 종속되지 않는 공용 전투 유틸리티 */
UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 무기 스윙·피니셔를 비롯한 대미지 경로가 공유하는 단일 진입점이다.
	 *
	 * 예측 키는 몽타주를 재생 중인 어빌리티의 활성화 키를 쓴다.
	 * 애님 중인 어빌리티가 없으면(시뮬 프록시 등) 키가 무효라 권위 머신에서만 적용된다.
	 *
	 * 다만 대미지 GE는 Instant+Execution이라 엔진이 예측 시 execution을 건너뛴다.
	 * 어트리뷰트는 서버 권위로 남고, 실제로 예측되는 것은 DamageInfo의 지속형 AdditionalEffects뿐이다.
	 *
	 * @param HitStopDuration	0보다 크고 적중이 성립하면 공격자 ASC의 히트스톱을 이 길이만큼 발동한다.
	 * @return					Spec 중 하나라도 권위 또는 예측으로 적용됐으면 true.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration = 0.f);

	/**
	 * 구간이 정해진 상태 GE(무적·퍼펙트가드 등)를 그 길이만큼 걸어 스스로 만료되게 한다.
	 *
	 * 정의의 지속시간 대신 Duration을 스펙에 잠가 싣는다.
	 * 잠그지 않으면 적용 단계에서 정의값이 다시 실려 구간이 닫히지 않는다.
	 *
	 * @param PredictingAbility	이 어빌리티의 활성화 예측 키로 적용해 소유 클라이언트도 같은 프레임에 태그를 갖는다. 널이면 서버에서만 걸리고 클라는 복제로 받는다.
	 */
	static FActiveGameplayEffectHandle ApplyEffectForDuration(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Duration, const UGameplayAbility* PredictingAbility);
};
