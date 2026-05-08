// Copyright Woogle. All Rights Reserved.

#include "WxEffectZone.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SceneComponent.h"

AWxEffectZone::AWxEffectZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AWxEffectZone::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	ApplyEffect(OtherActor);
}

void AWxEffectZone::ApplyEffect(AActor* Target)
{
	if (!Target || !EffectClass)
	{
		return;
	}

	if (bApplyOncePerTarget && AppliedTargets.Contains(Target))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC)
	{
		return;
	}

	// Instigator Pawn의 ASC를 SourceASC로 사용. 없으면 Target에 자가 적용
	APawn* InstigatorPawn = GetInstigator();
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorPawn);
	if (!SourceASC)
	{
		SourceASC = TargetASC;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddInstigator(InstigatorPawn, this);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 0, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	for (const TPair<FGameplayTag, float>& Pair : SetByCallers)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
	}

	if (!EffectAssetTags.IsEmpty())
	{
		SpecHandle.Data->AppendDynamicAssetTags(EffectAssetTags);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (bApplyOncePerTarget)
	{
		AppliedTargets.Add(Target);
	}
}
