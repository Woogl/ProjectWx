// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_CancelWindow.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

void UWxAnimNotifyState_CancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner)))
		{
			ASC->OpenCancelWindow(AllowedAbilities);
		}
	}
}

void UWxAnimNotifyState_CancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner)))
		{
			ASC->CloseCancelWindow();
		}
	}
}
