// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_StartRecovery.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "System/WxCombatDeveloperSettings.h"

FLinearColor UWxAnimNotify_StartRecovery::GetEditorColor()
{
	return GetDefault<UWxCombatDeveloperSettings>()->CombatAnimNotifyColor;
}

void UWxAnimNotify_StartRecovery::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	Recover(MeshComp, MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE);
}

void UWxAnimNotify_StartRecovery::BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	Recover(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.MontageInstanceID);
}

void UWxAnimNotify_StartRecovery::Recover(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr);
	if (UWxAbilityBase* Ability = ASC ? Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()) : nullptr)
	{
		Ability->StartRecovery(MontageInstanceID);
	}
}
