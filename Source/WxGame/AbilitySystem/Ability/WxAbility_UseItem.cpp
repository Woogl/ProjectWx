// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_UseItem.h"
#include "GameFramework/Pawn.h"
#include "Inventory/WxItemUseComponent.h"
#include "WxGameplayTags.h"

UWxAbility_UseItem::UWxAbility_UseItem()
{
	// NetExecutionPolicy 는 베이스의 LocalPredicted 를 그대로 쓴다 — 몽타주와 배타 점유가 입력 프레임에 즉시 걸려야 한다.

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_UseItem);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_UseItem);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 마시는 중에는 다른 어빌리티로 캔슬되지 않고, 후딜 캔슬로 비집고 들어왔을 때는 발동이 앞 액션을 끊는다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;
}

void UWxAbility_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!UseMontage || !ConsumableDef)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APawn* Avatar = Cast<APawn>(ActorInfo->AvatarActor.Get());
	UWxItemUseComponent* ItemUseComponent = Avatar ? Avatar->FindComponentByClass<UWxItemUseComponent>() : nullptr;
	if (!ItemUseComponent || !ItemUseComponent->CanUseItem(ConsumableDef))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// GAS 는 취소로 커밋된 쿨다운·코스트를 되돌리지 않으므로, 모든 거부 조건을 통과한 뒤에 커밋한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ItemUseComponent->BeginUseItem(ConsumableDef);

	if (!PlayMontage(UseMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

}

void UWxAbility_UseItem::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UWxItemUseComponent* ItemUseComponent = Avatar ? Avatar->FindComponentByClass<UWxItemUseComponent>() : nullptr)
	{
		ItemUseComponent->EndUseItem(ConsumableDef);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
