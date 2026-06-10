// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_UseItem.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "WxGameplayTags.h"

UWxAbility_UseItem::UWxAbility_UseItem()
{
	// 인벤토리 차감은 서버 권한 작업이므로 서버에서 활성화한다(WxAbility_Interact 와 동일).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_UseItem);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 마시는 중에는 다른 어빌리티로 캔슬되지 않는다. 후딜 캔슬은 몽타주의 StartRecovery 노티파이로 허용한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
}

void UWxAbility_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!UseMontage || !ConsumableDef || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 보유·충전이 없으면 마시기 모션을 내지 않는다(빈 병 방지). 실제 사용 검증/차감은 노티파이 시점의 UseItemByDef 가 재수행.
	APawn* Avatar = Cast<APawn>(ActorInfo->AvatarActor.Get());
	UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(Avatar);
	if (!Inventory || !Inventory->CanUseItemByDef(ConsumableDef))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, UseMontage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_UseItem::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_UseItem::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_UseItem::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_UseItem::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 마시는 순간 노티파이(Event.UseItem)에 맞춰 실제 사용 처리. 1회만 수신한다.
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_UseItem, nullptr, true);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_UseItem::HandleConsumeEvent);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_UseItem::HandleConsumeEvent(FGameplayEventData Payload)
{
	// ServerInitiated 라 이 경로는 서버에서만 실행된다. UseItemByDef 가 충전 1 감소 + 회복 GE 적용을 원자적으로 수행한다.
	APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(Avatar))
	{
		Inventory->UseItemByDef(ConsumableDef);
	}
}

void UWxAbility_UseItem::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_UseItem::HandleMontageBlendOut()
{
	// OnCompleted 가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_UseItem::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_UseItem::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
