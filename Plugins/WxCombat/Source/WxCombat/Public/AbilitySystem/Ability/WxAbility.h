// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WxAbility.generated.h"

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
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXCOMBAT_API UWxAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnInputTriggered;

	/** 이 어빌리티를 활성화할 입력 태그. GiveAbility 시 DynamicAbilityTags에 추가됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	FGameplayTag ActivationInputTag;

	/** 쿨다운 지속 시간 (초). 0 이하이면 쿨다운 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cooldown")
	float CooldownDuration = 0.f;

	/** 쿨다운 중 ASC에 부여되는 태그. CheckCooldown에서 이 태그로 쿨다운 여부를 판단 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cooldown", meta = (EditCondition = "CooldownDuration > 0"))
	FGameplayTag CooldownTag;

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	mutable FGameplayTagContainer CooldownTagContainer;
};
