// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_ApplyEffectToSelf.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

void UWxAnimNotify_ApplyEffectToSelf::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !EffectClass)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!ASC)
	{
		return;
	}

	const UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	ASC->ApplyGameplayEffectToSelf(EffectCDO, EffectLevel, ASC->MakeEffectContext());
}

FString UWxAnimNotify_ApplyEffectToSelf::GetNotifyName_Implementation() const
{
	if (EffectClass)
	{
		return FString::Printf(TEXT("Effect: %s"), *EffectClass->GetName());
	}

	return TEXT("Apply Effect To Self");
}
