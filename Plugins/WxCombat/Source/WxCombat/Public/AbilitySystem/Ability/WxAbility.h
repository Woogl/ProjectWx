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

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
