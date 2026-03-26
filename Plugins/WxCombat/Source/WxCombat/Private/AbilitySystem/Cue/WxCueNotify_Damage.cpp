// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Damage.h"
#include "AbilitySystem/Cue/WxDamageFloaterActor.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"

UWxCueNotify_Damage::UWxCueNotify_Damage()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Damage;
}

void UWxCueNotify_Damage::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	if (!MyTarget || !FloaterWidgetClass)
	{
		return;
	}

	UWorld* World = MyTarget->GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWxDamageFloaterActor* FloaterActor = World->SpawnActor<AWxDamageFloaterActor>(Parameters.Location, FRotator::ZeroRotator, SpawnParams);
	if (FloaterActor)
	{
		const float Damage = Parameters.RawMagnitude;
		const bool bIsCritical = Parameters.AggregatedSourceTags.HasTag(WxGameplayTags::Damage_Critical);
		FloaterActor->InitDamageInfo(FloaterWidgetClass, Damage, bIsCritical);
	}

	if (HitNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitNiagaraSystem, Parameters.Location);
	}
}
