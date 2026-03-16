// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxGameplayTags.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWxAbilitySystemComponent::GiveAbilitySet()
{
	if (!AbilitySet)
	{
		return;
	}

	AbilitySet->GiveToAbilitySystem(this, &AbilitySetGrantedHandles);
}

void UWxAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	if (ComboHandleMap.Contains(InputTag))
	{
		RequestComboAttack(InputTag);
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = true;
			if (Spec.IsActive())
			{
				AbilitySpecInputPressed(Spec);
			}
			else if (TryActivateAbility(Spec.Handle))
			{
				break;
			}
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

// ── Combo ──────────────────────────────────────────────────────────────────

bool UWxAbilitySystemComponent::IsComboWindowOpen() const
{
	return HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow);
}

void UWxAbilitySystemComponent::RequestComboAttack(const FGameplayTag& InputTag)
{
	const TArray<FGameplayAbilitySpecHandle>* Handles = ComboHandleMap.Find(InputTag);
	if (!Handles || Handles->IsEmpty())
	{
		return;
	}

	if (IsComboWindowOpen() && ActiveComboInputTag == InputTag)
	{
		CurrentComboIndex++;
	}
	else
	{
		const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(ActiveComboHandle);
		if (Spec && Spec->IsActive())
		{
			return;
		}

		ActiveComboInputTag = InputTag;
		CurrentComboIndex = 1;
	}

	if (CurrentComboIndex > Handles->Num())
	{
		CurrentComboIndex = 0;
		return;
	}

	ActiveComboHandle = (*Handles)[CurrentComboIndex - 1];
	if (!TryActivateAbility(ActiveComboHandle))
	{
		CurrentComboIndex = 0;
	}
}

void UWxAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	const UWxAbility* WxAbility = Cast<UWxAbility>(AbilitySpec.Ability);
	if (!WxAbility || !WxAbility->bIsComboAbility || !WxAbility->ActivationInputTag.IsValid() || WxAbility->ComboIndex <= 0)
	{
		return;
	}

	// Handle 캐싱 (ComboIndex 순서, ActivationInputTag 기준)
	TArray<FGameplayAbilitySpecHandle>& Handles = ComboHandleMap.FindOrAdd(WxAbility->ActivationInputTag);
	if (Handles.Num() < WxAbility->ComboIndex)
	{
		Handles.SetNum(WxAbility->ComboIndex);
	}
	Handles[WxAbility->ComboIndex - 1] = AbilitySpec.Handle;
}

