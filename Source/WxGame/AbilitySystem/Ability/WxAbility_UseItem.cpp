// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_UseItem.h"
#include "GameFramework/Pawn.h"
#include "Inventory/WxItemUseComponent.h"
#include "WxGameplayTags.h"

UWxAbility_UseItem::UWxAbility_UseItem()
{
	// 베이스의 LocalPredicted를 유지해 몽타주와 어빌리티 태그 차단을 입력 프레임에 적용한다.

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_UseItem);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_UseItem);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 본동작은 공통 태그로 새 액션을 막고, 발동 시에는 앞 액션의 후딜을 취소한다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;
}

void UWxAbility_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GetMontage())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APawn* Avatar = Cast<APawn>(ActorInfo->AvatarActor.Get());
	UWxItemUseComponent* ItemUseComponent = Avatar ? Avatar->FindComponentByClass<UWxItemUseComponent>() : nullptr;
	if (!ItemUseComponent || !ItemUseComponent->CanUseItem())
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

	ItemUseComponent->BeginUseItem();

	if (!PlayMontage(GetMontage()))
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
		ItemUseComponent->EndUseItem();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
