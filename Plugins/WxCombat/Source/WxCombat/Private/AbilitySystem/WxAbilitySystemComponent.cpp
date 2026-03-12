// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicated(true);
}

void UWxAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	// 2-pass 방식으로 처리한다.
	// TryActivateAbility는 즉시 부수효과(CancelAbilitiesWithTag, ActivationOwnedTags 부여 등)를
	// 발생시키므로, 단일 루프 안에서 호출하면 같은 InputTag를 공유하는 후속 어빌리티의
	// 활성화 조건까지 충족시켜 연쇄 발동(예: Combo1→2→3→4)이 일어난다.
	//
	// 1차: InputPressed 마킹 및 활성 어빌리티 입력 전달, 비활성 핸들 수집
	// 2차: 수집된 핸들로 활성화 시도, 첫 성공 시 중단

	TArray<FGameplayAbilitySpecHandle> InactiveHandles;

	// 1차 — 어빌리티 리스트를 안전하게 순회하며 입력 상태만 갱신한다.
	{
		ABILITYLIST_SCOPE_LOCK();
		for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				Spec.InputPressed = true;
				if (Spec.IsActive())
				{
					AbilitySpecInputPressed(Spec);
				}
				else
				{
					InactiveHandles.Add(Spec.Handle);
				}
			}
		}
	}

	// 2차 — 복사된 핸들 배열로 활성화를 시도한다.
	// 첫 번째 성공 시 break하여 한 프레임에 하나의 어빌리티만 활성화되도록 보장한다.
	for (const FGameplayAbilitySpecHandle& Handle : InactiveHandles)
	{
		if (TryActivateAbility(Handle))
		{
			break;
		}
	}
}

void UWxAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = false;
			if (Spec.IsActive())
			{
				AbilitySpecInputReleased(Spec);
			}
		}
	}
}
