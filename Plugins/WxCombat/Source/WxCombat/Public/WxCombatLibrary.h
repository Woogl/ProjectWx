// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

struct FWxDamageInfo;
struct FHitResult;

/**
 * WxCombat 전용 Blueprint Function Library.
 * 무기/투사체에 종속되지 않는 공용 전투 유틸리티(대미지 적용, 적대 판정 등)를 제공한다.
 */
UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 한 건의 대미지를 단일 타겟에 즉시 적용한다.
	 *
	 * 절차: EffectContext 구성 → DamageInfo→Spec 변환 → Spec 적용 → (옵션) HitStop 큐.
	 * Source/Target에 ASC가 없으면 false 반환.
	 *
	 * **팀 검사는 하지 않는다.** 같은 편 대미지(광역 오발, PvP 등)가 필요한 경우를 위해
	 * 정책은 호출자 몫으로 둔다. 적대 액터에만 적용하려면 호출 전에 IsHostile()로 거른다.
	 *
	 * 광역 대미지·신체부위 공격·환경 트리거 등 무기/투사체 외 경로의 단일 진입점이다.
	 *
	 * @param Instigator        공격을 일으킨 캐릭터. ASC가 여기에 부착되어 있어야 한다.
	 * @param Target            피격 대상.
	 * @param DamageInfo        대미지 한 건의 데이터.
	 * @param HitResult         피격 위치 정보. EffectContext에 추가된다.
	 * @param SourceObject      Spec의 SourceObject로 기록될 객체(무기/투사체/볼륨 등). nullptr이면 Instigator.
	 * @param HitStopDuration   0보다 크면 HitStop GameplayCue를 함께 발동한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat", meta = (AdvancedDisplay = "SourceObject,HitStopDuration"))
	static bool ApplyDamage(AActor* Instigator, AActor* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, UObject* SourceObject = nullptr, float HitStopDuration = 0.f);

	/** Source가 Target을 적대 관계로 보는지 검사 (IGenericTeamAgentInterface 기반). 인터페이스가 없으면 true 반환. */
	UFUNCTION(BlueprintPure, Category = "Wx|Combat")
	static bool IsHostile(const AActor* Source, const AActor* Target);
};
