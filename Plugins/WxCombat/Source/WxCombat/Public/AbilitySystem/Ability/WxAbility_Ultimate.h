// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Ultimate.generated.h"

class UAnimMontage;
class ULevelSequence;
struct FStreamableHandle;

/**
 * 궁극기 어빌리티.
 * 컷신(Level Sequence)을 재생한 뒤 공격 몽타주를 실행하며, 컷신 동안 월드는 시간 정지하고 캐릭터는 무적이 된다.
 *
 * 컷신 시퀀스는 부여 시점에 비동기로 미리 잡아 둔다.
 * LevelSequence는 카메라·머티리얼·사운드까지 함께 끌고 오므로, 발동 순간에 로드하면 하필 연출이 시작되는 지점에서 프레임이 튄다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Ultimate : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Ultimate();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TSoftObjectPtr<ULevelSequence> CutsceneSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> UltimateMontage;

private:
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

	/** 이 핸들이 곧 GC 방지이므로 어빌리티가 살아 있는 동안 들고 있는다 */
	TSharedPtr<FStreamableHandle> CutscenePreloadHandle;
};
