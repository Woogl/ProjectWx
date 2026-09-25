// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/ActiveMontageInstanceScope.h"
#if WITH_EDITOR
#include "WxAnimNotifySettings.h"
#endif
#include "WxCombatLibrary.h"
#include "WxCombatModule.h"

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_ApplyGameplayEffect::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->EffectColor;
}
#endif

void UWxAnimNotifyState_ApplyGameplayEffect::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	BeginWindow(MeshComp, MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE);
}

void UWxAnimNotifyState_ApplyGameplayEffect::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	EndWindow(MeshComp, MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE);
}

void UWxAnimNotifyState_ApplyGameplayEffect::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	BeginWindow(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.MontageInstanceID);
}

void UWxAnimNotifyState_ApplyGameplayEffect::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	EndWindow(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.MontageInstanceID);
}

FString UWxAnimNotifyState_ApplyGameplayEffect::GetNotifyName_Implementation() const
{
	FString ClassName = EffectClass ? EffectClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Effect: %s"), *ClassName);
}

void UWxAnimNotifyState_ApplyGameplayEffect::BeginWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	if (MontageInstanceID == INDEX_NONE)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("ApplyGameplayEffect 노티파이는 몽타주에서만 동작한다. 끝에서 걷을 구간을 가를 수 없어 적용하지 않는다. Notify=%s Owner=%s"), *GetNotifyName(), *GetNameSafe(Owner));
		return;
	}

	const FActiveGameplayEffectHandle Handle = UWxCombatLibrary::ApplyEffect(ASC, EffectClass, ASC->GetAnimatingAbility());
	if (Handle.IsValid())
	{
		AppliedEffects.Add(MontageInstanceID, Handle);
	}
}

void UWxAnimNotifyState_ApplyGameplayEffect::EndWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID)
{
	FActiveGameplayEffectHandle Handle;
	if (!AppliedEffects.RemoveAndCopyValue(MontageInstanceID, Handle))
	{
		return;
	}

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->RemoveActiveGameplayEffect(Handle, 1);
	}
}
