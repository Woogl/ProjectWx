// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxDamageInfo.h"
#include "WxAbility_Finisher.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
struct FGameplayEventData;

/**
 * 피니셔 어빌리티 — 공격자(플레이어) 측. 한 클래스가 두 변형을 트리거 EventTag로 분기한다.
 *  - 앞잡(피니셔): Event.Finisher 트리거. 적 정렬, 짝 피격 Event.HitReact.Finisher, DamageInfo 피해(+DP 0 으로 그로기 해제).
 *  - 뒤잡(백스탭): Event.Backstab 트리거. 적 정렬, 짝 피격 Event.HitReact.Backstab, 치명 대미지(즉사).
 *
 * 공통 발동 흐름:
 *  1. 상호작용(서버 권위)이 보내는 GameplayEvent 로 트리거(Target=적)
 *  2. 대상 적을 모션워핑 워프 타겟으로 등록(접근·정렬)
 *  3. 적에게 변형별 짝 피격 이벤트 송출 → 적이 짝 피격 몽타주를 동시 재생
 *  4. 변형별 공격자 몽타주 재생(몽타주의 MotionWarping 노티파이가 워프 타겟으로 정렬)
 *  5. 몽타주 후반 노티파이가 변형별 대미지 타이밍 이벤트 송출 → 대상 적에 피해 적용
 *  6. 몽타주 종료 시 EndAbility
 *
 * 앞잡 DamageInfo 의 피해 GE 에 DP Override 0 모디파이어를 포함하면, 피해 적용 시 DP 가 0 이 되어
 * WxCombatAttributeSet 이 State.Groggy 를 제거하고 그로기가 해제된다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Finisher : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Finisher();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 앞잡(그로기 피니시) 공격자 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> FinisherMontage;

	/** 앞잡 대상 적에 적용할 피해 정보. 피해 GE 에 DP Override 0 모디파이어를 포함해 그로기를 해제한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	FWxDamageInfo DamageInfo;

	/** 뒤잡(백스탭) 공격자 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> BackstabMontage;

	/** 뒤잡 치명 대미지(고정). 대상 HP 와 무관히 즉사하도록 큰 값으로 둔다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0"))
	float BackstabDamage = 99999.f;

	/** 워프 정지 거리(cm). 대상 적 이 거리에서 멈춘다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0"))
	float WarpDistance = 100.f;

private:
	UFUNCTION()
	void HandleDamageEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	void RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const;

	/** 앞잡 피해(DamageInfo, DP 0 으로 그로기 해제). */
	void ApplyFinisherDamage() const;

	/** 뒤잡 치명 피해(BackstabDamage, 즉사). */
	void ApplyBackstabDamage() const;

	/** 워프 타겟 이름. 두 변형(앞잡·뒤잡)의 공격 몽타주 MotionWarping 노티파이 Warp Target Name 을 이 값으로 맞춘다. */
	static const FName WarpTargetName;

	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> DamageEventTask;
};
