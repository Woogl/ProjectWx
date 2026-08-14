// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_UseItem.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "WxGameplayTags.h"

UWxAbility_UseItem::UWxAbility_UseItem()
{
	// NetExecutionPolicy 는 베이스의 LocalPredicted 를 그대로 쓴다 — 몽타주와 블로킹 태그가 입력 프레임에 즉시 걸려야 한다.

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_UseItem);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_UseItem);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 마시는 중에는 다른 어빌리티로 캔슬되지 않는다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	// 사용 시작 시 진행 중이던 액션을 끊는다(마시면서 스프린트 속도로 이동하는 것 등을 방지).
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
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
	UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(Avatar);
	if (!Inventory || !Inventory->CanUseItemByDef(ConsumableDef))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 커밋은 모든 거부 조건을 통과한 뒤에 한다.
	// GAS 는 취소로 커밋된 쿨다운·코스트를 되돌리지 않는다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	// 1회만 수신한다.
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
	// 몽타주가 클라에서도 재생되어 노티파이가 양쪽에서 발화하므로, 이 경로는 클라 인스턴스에서도 도달한다.
	// 인벤토리 차감은 롤백 장치가 없어 예측 대상이 아니다(UseItemByDef 는 권한을 check 한다).
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

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
	// OnCompleted 가 후속 발동하므로 여기서는 처리하지 않는다.
}

void UWxAbility_UseItem::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_UseItem::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
