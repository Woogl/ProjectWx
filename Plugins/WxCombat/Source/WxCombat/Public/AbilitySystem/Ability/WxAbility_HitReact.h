// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 피격 반응 어빌리티. (일반 / 넉백 / 넉다운 / 넉업 / 패리 / 피니셔(앞잡) / 백스탭(뒤잡))
 * 데미지 파이프라인이 보내는 Event.HitReact.* 로 트리거되어 그 태그에 매칭되는 몽타주를 재생한다.
 *
 * 평타로는 경직이 나지 않는다 — 대미지 행이 Event.HitReact.* 태그를 싣지 않으면 이벤트 자체가 발송되지 않기 때문이다.
 * 가드 중 피격 반응은 WxAbility_Guard가 직접 처리하므로 State.Guard 중에는 이 어빌리티가 뜨지 않는다.
 * 피격 중에도 새 액션을 허용하는 캐릭터는 어빌리티 BP에서 차단·캔슬 컨테이너를 비운다(GA_HitReact_Custer).
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
	/** 매칭되는 몽타주가 없을 때의 폴백이기도 하다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> NormalHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockbackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockupMontage;

	/** 공격이 퍼펙트 가드로 막혀 공격자가 경직될 때 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> ParryReactMontage;

	/** 피니셔(앞잡) 짝 피격 몽타주. 공격자 몽타주와 프레임 싱크되도록 같은 길이로 제작한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> FinisherHitReactMontage;

	/** 백스탭(뒤잡) 짝 피격 몽타주. 공격자를 향해 회전한 뒤 재생한다. */
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
