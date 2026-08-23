// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_ComboWindow.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void UWxAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	if (UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()))
	{
		Ability->OpenComboWindow();
	}
}

void UWxAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	// 몽타주가 끊겨 다른 어빌리티가 재생 중이면 그쪽은 콤보 창이 아니라 닫힘이 no-op이 된다.
	if (UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()))
	{
		Ability->CloseComboWindow();
	}
}
