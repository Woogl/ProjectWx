// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 피격 반응 어빌리티. (일반 / 넉백 / 넉다운 / 넉업 / 패리 / 피니셔(앞잡) / 백스탭(뒤잡))
 *
 * 사용 흐름:
 *  1. 데미지 수신 → Event.HitReact.[Normal|Knockback|Knockdown|Knockup|Parry] 이벤트 발송
 *  2. GameplayEvent 트리거 → ActivateAbility
 *  3. 트리거 태그에 매칭되는 몽타주 재생 → 완료 시 EndAbility
 *
 * 피격은 반응이 끝날 때까지 새 액션(Ability.Exclusive)을 차단하고, 진행 중인 것 중에서는 공격·스킬만 끊는다.
 * 캔슬이 마커가 아니라 공격·스킬을 좁게 지목하는 것은 마커를 가진 적 패턴이 평타 피격에 중단되면 안 되기 때문이다.
 * 피격 중에도 새 액션을 허용하는 캐릭터는 어빌리티 BP에서 차단·캔슬 컨테이너를 비운다(GA_HitReact_Custer).
 *
 * 평타로는 경직이 나지 않는다 — 대미지 행이 Event.HitReact.* 태그를 싣지 않으면 이벤트 자체가 발송되지 않기 때문이다.
 *
 * 재생 중 다른 종류의 HitReact 이벤트가 도착하면(예: Normal 재생 중 Knockback), bRetriggerInstancedAbility로 EndAbility 후 ActivateAbility가 재진입하며, CurrentMontageTask를 명시적으로 정리해 이전 태스크의 잔여 콜백이 새 재생을 즉시 종료시키는 레이스를 방지한다.
 *
 * 가드 중 피격 반응은 WxAbility_Guard가 직접 처리하므로, State.Guard 활성 시 이 어빌리티는 활성화되지 않는다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	/** 기본 피격 반응 몽타주. Event.HitReact 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> NormalHitReactMontage;

	/** 넉백 몽타주. Event.HitReact.Knockback 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockbackMontage;

	/** 넉다운 몽타주. Event.HitReact.Knockdown 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	/** 넉업 몽타주. Event.HitReact.Knockup 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockupMontage;

	/** 패리 피격 몽타주. 공격이 퍼펙트 가드로 막혀 공격자가 경직될 때 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> ParryReactMontage;

	/** 피니셔(앞잡) 짝 피격 몽타주. Event.HitReact.Finisher 트리거 시 재생. 공격자 피니셔 몽타주와 프레임 싱크되도록 같은 길이로 제작 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> FinisherHitReactMontage;

	/** 백스탭(뒤잡) 짝 피격 몽타주. Event.HitReact.Backstab 트리거 시 재생. 피해자는 공격자(플레이어)를 향해 회전한 뒤 재생한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> BackstabHitReactMontage;

private:
	bool PlayHitReactMontage(UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
	
	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator);
};
