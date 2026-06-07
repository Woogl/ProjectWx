// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Pattern_Phase.generated.h"

class UAnimMontage;
class ULevelSequence;

/**
 * 적 페이즈 전환 어빌리티.
 *
 * BT의 WxBTTask_ActivateAbility가 AssetTag(Ability.Pattern.Phase)로 직접 발동한다.
 * 발동 시작 시 Blackboard의 Phase 값을 NextPhase로 변경시키고 몽타주를 실행한다.
 *
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Pattern_Phase : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Pattern_Phase();

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 컷신 이후 재생할 전환 몽타주. 비워두면 몽타주 단계를 건너뛴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> TransitionMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	int32 NextPhase = 2;

private:
	/** 아바타의 AIController Blackboard에서 Phase 값을 바꾼다. */
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
