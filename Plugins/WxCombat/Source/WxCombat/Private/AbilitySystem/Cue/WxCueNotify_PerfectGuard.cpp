// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_PerfectGuard.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"

UWxCueNotify_PerfectGuard::UWxCueNotify_PerfectGuard()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_PerfectGuard;
}

void UWxCueNotify_PerfectGuard::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed || !MyTarget)
	{
		return;
	}

	UWorld* World = MyTarget->GetWorld();
	if (!World)
	{
		return;
	}

	if (HitNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitNiagaraSystem, Parameters.Location);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, HitSound, Parameters.Location);
	}
}
