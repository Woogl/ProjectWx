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

/** 무기·투사체에 종속되지 않는 공용 전투 유틸리티 */
UCLASS()
class WXCOMBAT_API UWxCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 판정이 성립하는 관계인지 가른다 — 적대에만 성립하며 아군·중립에는 대미지도 연출도 발생하지 않는다.
	 * 무기·투사체가 히트를 채택할지 정하는 기준이자 대미지 ExecCalc의 선판정이다.
	 * 판정 근거가 없으면(둘 중 하나가 널이거나 공격자에게 팀 개념이 없으면) 성립으로 본다.
	 */
	static bool IsHostile(const AActor* Source, const AActor* Target);

	/**
	 * 무기 스윙·피니셔를 비롯한 대미지 경로가 공유하는 단일 진입점이자, 적중 성립 판정을 소비하는 유일한 자리다.
	 * 성립하지 않는 히트(아군·중립·시체·회피 무적)는 GE를 하나도 걸지 않으므로, 대미지 GE에 얹힌 히트 큐도 상태이상도 발생하지 않는다.
	 * 회피만은 어트리뷰트에 흔적이 남지 않아 사후 판별이 불가능하므로, 여기서 회피 성공 이벤트를 대신 발행한다.
	 *
	 * 예측 키는 몽타주를 재생 중인 어빌리티의 활성화 키를 쓴다.
	 * 애님 중인 어빌리티가 없으면(시뮬 프록시 등) 키가 무효라 권위 머신에서만 적용된다.
	 *
	 * 다만 대미지 GE는 Instant+Execution이라 엔진이 예측 시 execution을 건너뛴다.
	 * 어트리뷰트는 서버 권위로 남고, 실제로 예측되는 것은 DamageTableRow의 지속형 AdditionalEffects뿐이다.
	 *
	 * @param HitStopDuration	0보다 크고 적중이 성립하면 공격자 ASC의 히트스톱을 이 길이만큼 발동한다.
	 * @return					Spec 중 하나라도 권위 또는 예측으로 적용됐으면 true.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Combat")
	static bool ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration = 0.f);

	/**
	 * 구간을 여는 쪽이 수명을 소유하는 상태 GE(무적·퍼펙트가드 등)를 건다. 구간을 닫을 때 RemoveEffect와 짝으로 쓴다.
	 *
	 * @param PredictingAbility	이 어빌리티의 활성화 예측 키로 적용해 소유 클라이언트도 같은 프레임에 태그를 갖는다. 널이면 서버에서만 걸리고 클라는 복제로 받는다.
	 */
	static void ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility);

	/**
	 * ApplyEffect로 연 구간을 닫는다.
	 *
	 * 예측으로 건 GE의 핸들은 서버본이 도착하면 무효해지므로 정의로 조회해 지운다.
	 * 다만 하나만 걷어낸다 — 같은 GE를 건 다른 출처(처형의 활성 구간, 궁극기 컷신)가 겹쳐 있어도 그쪽 무적까지 벗기지 않기 위해서다.
	 */
	static void RemoveEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass);
};
