// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

class UAbilitySystemComponent;
struct FWxDamageInfo;
struct FHitResult;

/**
 * WxCombat 전용 Blueprint Function Library.
 * 무기/투사체에 종속되지 않는 공용 전투 유틸리티를 제공한다.
 */
UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 한 건의 대미지를 단일 타겟에 즉시 적용한다.
	 *
	 * 절차: EffectContext 구성 → DamageInfo→Spec 변환 → HitStop SetByCaller 부착 → Spec 적용.
	 * Source/Target ASC 가 nullptr 이거나 둘이 동일하면 false 반환.
	 *
	 * 광역 대미지·신체부위 공격·환경 트리거 등 무기/투사체 외 경로의 단일 진입점이다.
	 *
	 * @param Source			공격을 일으킨 액터의 ASC.
	 * @param Target			피격 대상의 ASC.
	 * @param DamageInfo		대미지 한 건의 데이터.
	 * @param HitResult			피격 위치 정보. EffectContext에 추가된다.
	 * @param HitStopDuration	0보다 크면 SetByCaller.HitStop으로 실어, WxExecCalc_Damage가 적중 시 공격자에게 HitStop을 발동한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration = 0.f);
};
