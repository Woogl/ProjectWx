// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Finisher.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
struct FGameplayEventData;
struct FWxDamageInfo;

/**
 * 피니셔 어빌리티 — 공격자(플레이어) 측. 한 클래스가 두 변형을 트리거 EventTag로 분기한다.
 *  - 앞잡(피니셔): Event.Finisher 트리거. 적 정렬, 짝 피격 Event.HitReact.Finisher.
 *  - 뒤잡(백스탭): Event.Backstab 트리거. 적 정렬, 짝 피격 Event.HitReact.Backstab.
 *
 * 공통 발동 흐름:
 *  1. 상호작용(서버 권위)이 보내는 GameplayEvent 로 트리거(Target=적)
 *  2. 플레이어 위치를 대상 적 현재 위치 앞으로 모션워핑 정렬(회전은 플레이어 방향 유지; 위치 기준=몬스터, 각도 기준=플레이어)
 *  3. 적에게 변형별 짝 피격 이벤트 송출 → 적이 공격자를 향해 회전 후 짝 피격 몽타주를 동시 재생
 *  4. 변형별 공격자 몽타주 재생(몽타주의 MotionWarping 노티파이가 워프 타겟으로 정렬)
 *  5. 몽타주의 WxAnimNotify_FinisherDamage 가 ApplyFinisherDamage 를 호출 → 확정 대상에 피해 적용
 *  6. 몽타주 종료 시 EndAbility
 *
 * 대미지 수치는 어빌리티가 아니라 공격 몽타주의 WxAnimNotify_FinisherDamage 가 대미지 테이블 행으로 입력한다.
 * 어빌리티는 상호작용으로 확정한 대상에 그 피해를 적용만 하므로 앞잡·뒤잡이 동일 경로로 통일된다.
 * 앞잡의 그로기 해제(DP 0)는 해당 대미지 행의 AdditionalEffects 에 DP Override 0 GE 를 포함해 처리한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Finisher : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Finisher();

	/**
	 * 상호작용으로 확정한 대상에 주어진 피해를 적용한다. 공격 몽타주의 WxAnimNotify_FinisherDamage 가
	 * 대미지 테이블 행을 해석해 호출한다(대미지 타이밍·수치는 노티파이가 결정, 어빌리티는 대상·적용만 담당).
	 */
	void ApplyFinisherDamage(const FWxDamageInfo& DamageInfo) const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 앞잡(그로기 피니시) 공격자 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> FinisherMontage;

	/** 뒤잡(백스탭) 공격자 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> BackstabMontage;

	/** 워프 정지 거리(cm). 대상 적 이 거리에서 멈춘다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0"))
	float WarpDistance = 100.f;

private:
	UFUNCTION()
	void HandleMontageFinished();

	void RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const;

	/** 워프 타겟 이름. 두 변형(앞잡·뒤잡)의 공격 몽타주 MotionWarping 노티파이 Warp Target Name 을 이 값으로 맞춘다. */
	static const FName WarpTargetName;

	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
};
