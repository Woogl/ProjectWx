// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_PhaseTransition.generated.h"

class UAnimMontage;
class ULevelSequence;

/**
 * 적 페이즈 전환 어빌리티.
 *
 * BT의 WxBTTask_ActivateAbility가 AssetTag(Ability.PhaseTransition)로 직접 발동한다.
 * 발동 시작 시 Blackboard의 Phase 값을 1 증가시키고, 컷신(Level Sequence)을 재생한 뒤
 * 이어서 전환 몽타주를 실행한다.
 *
 * 컷신 에셋이 없으면 컷신 단계를 건너뛰고 바로 몽타주를 재생한다.
 * 컷신 동안 게임 월드는 시간 정지(Global Time Dilation)되며, 아바타는 무적 상태가 된다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_PhaseTransition : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_PhaseTransition();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

	/** 전환 시 재생할 컷신. 비워두면 컷신 단계를 건너뛴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TSoftObjectPtr<ULevelSequence> CutsceneSequence;

	/** 컷신 이후 재생할 전환 몽타주. 비워두면 몽타주 단계를 건너뛴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> TransitionMontage;

private:
	/** 아바타의 AIController Blackboard에서 Phase 값을 1 증가시킨다. */
	void AdvancePhase();

	UFUNCTION()
	void HandleCutsceneCompleted();

	UFUNCTION()
	void HandleCutsceneCancelled();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
