// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_GuardReact.generated.h"

class UAnimMontage;

/**
 * 가드가 흡수한 히트의 연출을 맡는다 — 가드 피격·넉 계열 가드·가드 브레이크·퍼펙트 가드.
 *
 * 가드 어빌리티에서 떼어낸 이유는 복제다.
 * 대미지 GE가 피격자 클라에서는 예측 키가 없어 적용되지 않으니 그 이벤트도 서버에만 남는데, ServerInitiated 트리거는 엔진이 페이로드째 소유 클라에 복제해 준다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_GuardReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_GuardReact();

	/** 페이즈 몽타주는 길이가 곧 연출 규칙이므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 완주가 아니라 블렌드아웃에서 끝낸다 — 가드가 자세를 되찾는 구간이 연출 꼬리와 겹쳐야 끊겨 보이지 않는다. */
	virtual void HandleMontageBlendOut() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

	/** Knockback/Knockup/Knockdown 공격을 가드했을 때 재생하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardKnockbackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> PerfectGuardMontage;

	/** 퍼펙트 가드 성공 시 적용할 GlobalTimeDilation 값 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.01"))
	float PerfectGuardSlowTimeDilation = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.0"))
	float PerfectGuardSlowTimeDuration = 0.4f;

private:
	UAnimMontage* SelectMontage(FGameplayTag TriggerTag, FGameplayTag ReactionTag) const;
};
