// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_ComboWindow.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "System/WxCombatDeveloperSettings.h"

FLinearColor UWxAnimNotifyState_ComboWindow::GetEditorColor()
{
	return GetDefault<UWxCombatDeveloperSettings>()->CombatAnimNotifyColor;
}

void UWxAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	OpenWindow(MeshComp, MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE);
}

void UWxAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	CloseWindow(MeshComp, MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE);
}

void UWxAnimNotifyState_ComboWindow::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	OpenWindow(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.MontageInstanceID);
}

void UWxAnimNotifyState_ComboWindow::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	CloseWindow(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.MontageInstanceID);
}

void UWxAnimNotifyState_ComboWindow::OpenWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr);
	if (UWxAbilityBase* Ability = ASC ? Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()) : nullptr)
	{
		Ability->OpenComboWindow(MontageInstanceID);
	}
}

void UWxAnimNotifyState_ComboWindow::CloseWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr);
	if (UWxAbilityBase* Ability = ASC ? Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()) : nullptr)
	{
		Ability->CloseComboWindow(MontageInstanceID);
	}
}

FString UWxAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}
