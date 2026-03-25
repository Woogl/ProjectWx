// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxGameplayCueNotify_Damage.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Cue/WxDamageFloaterActor.h"

UWxGameplayCueNotify_Damage::UWxGameplayCueNotify_Damage()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Damage;
}

void UWxGameplayCueNotify_Damage::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	if (!MyTarget || !FloaterActorClass)
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
	AWxDamageFloaterActor* FloaterActor = World->SpawnActor<AWxDamageFloaterActor>(FloaterActorClass, Parameters.Location, FRotator::ZeroRotator, SpawnParams);
	if (FloaterActor)
	{
		const float Damage = Parameters.RawMagnitude;
		const bool bIsCritical = Parameters.AggregatedSourceTags.HasTag(WxGameplayTags::Damage_Critical);
		FloaterActor->InitDamageInfo(Damage, bIsCritical);
	}

	if (HitNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitNiagaraSystem, Parameters.Location);
	}
}
