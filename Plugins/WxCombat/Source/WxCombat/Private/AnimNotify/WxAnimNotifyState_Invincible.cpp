// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_Invincible.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void UWxAnimNotifyState_Invincible::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	// TotalDuration은 애니메이션 시간이라, ASPD로 재생 속도가 바뀐 몽타주에서는 GE의 실시간 지속시간과 어긋난다.
	float PlayRate = 1.f;
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
		const float EffectiveRate = AnimInstance ? FMath::Abs(AnimInstance->Montage_GetEffectivePlayRate(Montage)) : 0.f;
		if (EffectiveRate > UE_KINDA_SMALL_NUMBER)
		{
			PlayRate = EffectiveRate;
		}
	}

	UWxEffect_Invincible::ApplyTo(ASC, TotalDuration / PlayRate, ASC->GetAnimatingAbility());
}
