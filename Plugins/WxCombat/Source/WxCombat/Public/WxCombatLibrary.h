// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

class UAbilitySystemComponent;
struct FWxDamageInfo;
struct FHitResult;

/** 무기·투사체에 종속되지 않는 공용 전투 유틸리티 */
UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 한 건의 대미지를 단일 타겟에 즉시 적용한다.
	 *
	 * 무기 스윙·피니셔를 비롯한 대미지 경로가 공유하는 단일 진입점이다.
	 *
	 * 예측 키는 몽타주를 재생 중인 어빌리티의 활성화 키를 쓴다.
	 * 클라와 서버가 같은 키를 들고 있어 클라 적용이 예측이 되고, 서버 확정본이 도착하면 GAS가 예측본을 정리한다.
	 * 애님 중인 어빌리티가 없으면(시뮬 프록시 등) 키가 무효라 권위 머신에서만 적용된다.
	 *
	 * 다만 대미지 GE는 Instant+Execution이라 엔진이 예측 시 execution을 건너뛴다.
	 * 어트리뷰트와 ExecCalc 부수효과는 서버 권위로 남고, 실제로 예측되는 것은 DamageInfo의 지속형 AdditionalEffects뿐이다.
	 *
	 * @param HitStopDuration	0보다 크면 SetByCaller.HitStop으로 실어, 피격자 ASC가 적중 시 공격자에게 HitStop을 발동한다.
	 * @return					Spec 중 하나라도 권위 또는 예측으로 적용됐으면 true.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration = 0.f);
};
