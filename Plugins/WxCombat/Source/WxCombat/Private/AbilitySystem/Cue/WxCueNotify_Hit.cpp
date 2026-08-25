// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Hit.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"

UWxCueNotify_Hit::UWxCueNotify_Hit()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Hit;
}

void UWxCueNotify_Hit::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
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

	if (CameraShake)
	{
		// 큐는 서버·클라 양쪽에서 실행되므로 각 머신은 자기 로컬 컨트롤러만 흔든다. 리슨 서버가 원격 클라에 RPC까지 쏘면 셰이크가 두 번 걸린다.
		if (const APawn* Attacker = Cast<APawn>(Parameters.EffectContext.GetEffectCauser()))
		{
			if (APlayerController* PlayerController = Cast<APlayerController>(Attacker->GetController()))
			{
				if (PlayerController->IsLocalController())
				{
					PlayerController->ClientStartCameraShake(CameraShake);
					return;
				}
			}
		}

		if (const APawn* Victim = Cast<APawn>(MyTarget))
		{
			if (APlayerController* PlayerController = Cast<APlayerController>(Victim->GetController()))
			{
				if (PlayerController->IsLocalController())
				{
					PlayerController->ClientStartCameraShake(CameraShake);
				}
			}
		}
	}
}
