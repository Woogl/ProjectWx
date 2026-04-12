// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_HitStop.h"

#include "WxGameplayTags.h"
#include "GameFramework/Character.h"

UWxCueNotify_HitStop::UWxCueNotify_HitStop()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_HitStop;
}

void UWxCueNotify_HitStop::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(MyTarget);
	if (!Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance || !AnimInstance->GetCurrentActiveMontage())
	{
		return;
	}

	const float Duration = Parameters.RawMagnitude;

	AnimInstance->Montage_SetPlayRate(nullptr, 0.001f);

	// 파괴된 캐릭터의 잔여 엔트리 정리
	for (auto It = ActiveTimerMap.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	TWeakObjectPtr<ACharacter> WeakCharacter(Character);
	FTimerHandle& TimerHandle = ActiveTimerMap.FindOrAdd(Character);
	Character->GetWorldTimerManager().SetTimer(TimerHandle,
		FTimerDelegate::CreateWeakLambda(AnimInstance, [this, AnimInstance, WeakCharacter]()
		{
			AnimInstance->Montage_SetPlayRate(nullptr, 1.f);
			ActiveTimerMap.Remove(WeakCharacter);
		}),
		Duration, false);
}
