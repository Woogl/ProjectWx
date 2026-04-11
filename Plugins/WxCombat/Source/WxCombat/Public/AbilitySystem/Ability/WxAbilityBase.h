// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WxAbilityBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/** 어빌리티 활성화 정책 */
UENUM(BlueprintType)
enum class EWxAbilityActivationPolicy : uint8
{
	/** 입력 트리거 시 활성화 */
	OnInputTriggered,
	/** 부여(Grant)될 때 즉시 자동 활성화 (패시브, 상시 버프 등) */
	OnGranted,
};

/**
 * 프로젝트 전체 어빌리티 베이스 클래스.
 * 모든 어빌리티는 이 클래스를 상속받아 작성.
 *
 * 쿨다운은 GAS 표준 방식을 따른다 — 어빌리티 BP의 CooldownGameplayEffectClass 슬롯에
 * UWxEffect_CooldownBase 기반 BP 에셋을 지정하면 자동으로 동작한다. 충전(Charge) 시스템은
 * 해당 GE의 Stack Limit Count로 표현된다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnInputTriggered;

	/** 이 어빌리티를 활성화할 입력 태그. GiveAbility 시 DynamicAbilityTags에 추가됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx", meta = (Categories = "Input"))
	FGameplayTag ActivationInputTag;

	/** 어빌리티 아이콘. UI에서 표시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티 발동 시 자신에게 적용할 GameplayEffect 목록 (버프, 상태 부여 등). 각 GE의 Duration 정책에 따라 자연 만료된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TSubclassOf<UGameplayEffect>> OnActivateEffects;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
};
