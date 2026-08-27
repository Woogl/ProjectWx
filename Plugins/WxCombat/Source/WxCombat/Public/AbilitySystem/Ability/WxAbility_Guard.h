// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;

/**
 * 페이즈 (재생 중인 몽타주로 판별):
 *  GuardMontage          – Effect.Guard 활성, 루핑
 *  GuardHitReactMontage  – 일반 가드 피격(SP 여유) 후 GuardMontage 복귀
 *  GuardKnockbackMontage – Knock 계열 피격 가드 후 GuardMontage 복귀
 *  GuardBreakMontage     – SP 고갈로 Effect.Guard 해제 후 재생, 종료
 *  PerfectGuardMontage   – 퍼펙트 가드 성공 후 재생, 완주하면 종료(가드 키를 쥐고 있으면 가드가 새로 발동해 복귀)
 *                          완주 전에 후속 피격이 오면 리액션 페이즈로 끊긴다
 *
 * Damage.CanGuard가 없는 피격은 퍼펙트 가드 윈도우 중이라도 가드로 막히지 않는다.
 * UWxCombatAttributeSet::ProcessDamageTaken이 이 어빌리티를 Cancel한 뒤 Event.Hit을 보낸다.
 *
 * 가드 반격은 여기서 다루지 않는다 — Effect.Guard만 발행하면 공격 어빌리티가 그 태그로 자기 반격 세트를 고른다.
 * 진입 시점은 가드 몽타주의 StartRecovery가 차단을 푸는 때다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Guard : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	/** 홀드 입력이라 키가 눌려 있을 때만 시작한다. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** 페이즈 몽타주는 길이가 곧 연출 규칙이므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 리액션 페이즈가 끝나면 가드 페이즈로 되돌아간다. */
	virtual void HandleMontageBlendOut() override;

	/** 루핑하는 가드 페이즈에서는 종료하지 않는다. */
	virtual void HandleMontageCompleted() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

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
	void ListenForGuardHit();
	void ListenForPerfectGuard();

	UFUNCTION()
	void HandleHit(FGameplayEventData Payload);

	UFUNCTION()
	void HandlePerfectGuard(FGameplayEventData Payload);
};
