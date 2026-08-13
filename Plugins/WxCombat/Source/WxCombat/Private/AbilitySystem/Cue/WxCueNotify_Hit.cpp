// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Hit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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

	// 서버는 회피 히트에 출력 모디파이어가 없어 애초에 발행하지 않지만, 예측 클라는 ExecCalc을 건너뛰고 낙관 발행한다.
	// 이 검사가 없으면 회피에 성공해도 공격자 화면에만 스파크가 튄다. 무적 태그는 ANS가 각 머신에 로컬로 붙인다.
	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyTarget);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Invincible))
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
