// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SlowTime.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void UWxAnimNotifyState_SlowTime::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	UGameplayAbility* Ability = ASC ? ASC->GetAnimatingAbility() : nullptr;
	if (!Ability)
	{
		return;
	}

	// TotalDuration은 애니메이션 자체의 초라, 공격 속도로 배속된 재생에서는 구간이 그만큼 짧게 지나간다.
	const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	const float PlayRate = AnimInstance ? AnimInstance->Montage_GetPlayRate(Cast<UAnimMontage>(Animation)) : 0.f;
	const float SlowDuration = PlayRate > 0.f ? TotalDuration / PlayRate : TotalDuration;

	UWxAbilityTask_SlowTime::CreateTask(Ability, TimeDilation, SlowDuration)->ReadyForActivation();
}

FString UWxAnimNotifyState_SlowTime::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Slow Time (x%.2f)"), TimeDilation);
}
