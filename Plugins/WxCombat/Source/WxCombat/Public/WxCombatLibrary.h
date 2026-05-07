// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxCombatLibrary.generated.h"

class UAbilitySystemComponent;
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
	 * 절차: EffectContext 구성 → DamageInfo→Spec 변환 → Spec 적용 → HitStop 큐.
	 * Source/Target ASC 가 nullptr 이거나 둘이 동일하면 false 반환.
	 *
	 * 광역 대미지·신체부위 공격·환경 트리거 등 무기/투사체 외 경로의 단일 진입점이다.
	 *
	 * @param Source			공격을 일으킨 액터의 ASC.
	 * @param Target			피격 대상의 ASC.
	 * @param DamageInfo		대미지 한 건의 데이터.
	 * @param HitResult			피격 위치 정보. EffectContext에 추가된다.
	 * @param HitStopDuration	0보다 크면 HitStop GameplayCue를 함께 발동한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration = 0.f);

	/**
	 * 고정 대미지를 단일 타겟에 즉시 적용한다.
	 *
	 * ApplyDamage 의 Raw 모드 — Source ATK·Target DEF 와 무관하게 DamageAmount 를 그대로 BaseDamage 로 사용한다.
	 * 환경 대미지(낙사·트랩·도트 디버프 등) 처럼 정량 대미지가 필요한 경로에 적합하다.
	 *
	 * 대미지 GameplayCue 의 위치는 Target 액터의 월드 좌표가 사용된다.
	 *
	 * @param Target            피격 대상의 ASC.
	 * @param DamageAmount      원본 그대로 들어갈 대미지.
	 * @param HitReactionTag    적중 시 부여할 HitReact 태그. None 이면 HitReact 이벤트 미발송.
	 *                          Knockback/Knockdown/Knockup 등을 지정하면 PP 잔량과 무관하게 강제 발동된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyRawDamage(UAbilitySystemComponent* Target, float DamageAmount, UPARAM(meta = (Categories = "Event.HitReact")) FGameplayTag HitReaction);

	/** Source가 Target을 적대 관계로 보는지 검사 (IGenericTeamAgentInterface 기반) */
	UFUNCTION(BlueprintPure, Category = "Wx|Combat")
	static bool IsHostile(const AActor* Source, const AActor* Target);
};
