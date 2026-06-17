// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "AbilitySystem/TargetData/WxAbilityTargetData_Interaction.h"
#include "AbilitySystemComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionComponent.h"
#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "WxGameplayTags.h"

UWxAbility_Interact::UWxAbility_Interact()
{
	// 클라가 선택을 읽어 서버로 전달해야 하므로 클라/서버 모두에서 활성화되는 LocalPredicted를 쓴다.
	// 단, 클라는 선택 전송만 하고 실행은 서버 권한에서만 하므로 결과를 예측하지는 않는다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Interact);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

void UWxAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		if (IsLocallyControlled())
		{
			// 리슨서버 호스트: 로컬 선택을 직접 읽어 즉시 실행(RPC 왕복 불필요).
			ExecuteInteract(GetLocalSelectedComponent(ActorInfo), ActorInfo);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		else
		{
			// 서버가 원격 클라를 처리: 선택 컴포넌트 TargetData 수신 후 HandleTargetDataReceived에서 실행.
			FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey());
			Delegate.AddUObject(this, &UWxAbility_Interact::HandleTargetDataReceived);
			ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
		}
	}
	else
	{
		// 원격 클라: 로컬 선택을 TargetData로 서버에 전송. 선택이 없으면 null을 보내 서버가 무동작 후 종료하게 한다.
		FGameplayAbilityTargetDataHandle DataHandle;
		FWxAbilityTargetData_Interaction* TargetData = new FWxAbilityTargetData_Interaction();
		TargetData->Component = GetLocalSelectedComponent(ActorInfo);
		DataHandle.Add(TargetData);

		ASC->CallServerSetReplicatedTargetData(
			Handle,
			ActivationInfo.GetActivationPredictionKey(),
			DataHandle,
			FGameplayTag(),
			ASC->ScopedPredictionKey);

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UWxAbility_Interact::HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	UWxInteractionComponent* Selected = nullptr;
	if (const FWxAbilityTargetData_Interaction* TargetData = static_cast<const FWxAbilityTargetData_Interaction*>(DataHandle.Get(0)))
	{
		Selected = TargetData->Component;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	ExecuteInteract(Selected, CurrentActorInfo);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UWxInteractionComponent* UWxAbility_Interact::GetLocalSelectedComponent(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const APlayerController* PlayerController = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UWxInteractionRegistrySubsystem* Registry = LocalPlayer ? LocalPlayer->GetSubsystem<UWxInteractionRegistrySubsystem>() : nullptr;
	return Registry ? Registry->GetSelectedComponent() : nullptr;
}

void UWxAbility_Interact::ExecuteInteract(UWxInteractionComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (Selected && Avatar)
	{
		Selected->TryInteract(Avatar);
	}
}
